#include "OrderMatcher.hpp"
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>

// namespace declarations
using namespace std;
using namespace spdlog;
using namespace NSOrderMatching;

// external definitions
std::atomic<unsigned long> orderCount{0};
std::atomic<unsigned long> nextOrder{0};
std::atomic<bool> dataExausted{false};  
std::atomic<bool> dataReady{false};     

std::mutex orderSyncMutex;
std::condition_variable orderSyncCond;
std::mutex exceptMutex;

// Replace global vector with high-performance order book
std::unique_ptr<HighPerformanceOrderBook> OrderMatching::orderBook = std::make_unique<HighPerformanceOrderBook>(INIT_ORDER_BOOK_SIZE);

// Fine-grained locking for stock queues
OrderMatching::StockQueueMap OrderMatching::buyMap;
OrderMatching::StockQueueMap OrderMatching::sellMap;

std::vector<ExceptionRecord> allExceptions;

// Thread pool for processing orders by stock
std::unique_ptr<OrderMatching::OrderProcessorThreadPool> OrderMatching::threadPool = 
    std::make_unique<OrderMatching::OrderProcessorThreadPool>();

// logger objects for OrderBook class
// asynchronous console logger
shared_ptr<logger> OrderMatching::clogger;
// asynchronous file logger
shared_ptr<logger> OrderMatching::elogger;

// Thread pool implementation
OrderMatching::OrderProcessorThreadPool::OrderProcessorThreadPool(size_t numThreads) {
    // Create worker threads
    for (size_t i = 0; i < numThreads; ++i) {
        threads_.emplace_back(&OrderProcessorThreadPool::workerThread, this);
    }
}

OrderMatching::OrderProcessorThreadPool::~OrderProcessorThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_ = true;
        condition_.notify_all();
    }
    
    // Join all threads
    for (auto& thread : threads_) {
        if (thread.joinable()) {
            thread.join();
        }
    }
}

void OrderMatching::OrderProcessorThreadPool::workerThread() {
    while (true) {
        std::string stock;
        
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            condition_.wait(lock, [this] { return stop_ || !taskQueue_.empty(); });
            
            if (stop_ && taskQueue_.empty()) {
                return;
            }
            
            stock = taskQueue_.back();
            taskQueue_.pop_back();
        }
        
        // Process the stock
        OrderMatching::processStockOrders(stock);
    }
}

std::future<void> OrderMatching::OrderProcessorThreadPool::submitTask(const std::string& stock) {
    auto task = std::make_shared<std::packaged_task<void()>>(
        [stock]() { OrderMatching::processStockOrders(stock); }
    );
    
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        taskQueue_.push_back(stock);
    }
    
    condition_.notify_one();
    return task->get_future();
}

OrderMatching::OrderMatching(){
	clogger = Logger::getLogger(); 
	elogger = Logger::getAsyncLogger();
	elogger->debug("OrderBook created with size : {}", INIT_ORDER_BOOK_SIZE);
}

// Copy constructor implementation
OrderMatching::OrderMatching(const OrderMatching& ordMatcher) {
    clogger = ordMatcher.clogger;
    elogger = ordMatcher.elogger;
    elogger->debug("OrderMatching copy constructor called");
}

// Move constructor implementation
OrderMatching::OrderMatching(OrderMatching&& ordMatcher) noexcept {
    clogger = std::move(ordMatcher.clogger);
    elogger = std::move(ordMatcher.elogger);
    elogger->debug("OrderMatching move constructor called");
}

// Copy assignment operator implementation
OrderMatching& OrderMatching::operator=(const OrderMatching& ordMatcher) {
    if (this != &ordMatcher) {
        clogger = ordMatcher.clogger;
        elogger = ordMatcher.elogger;
        elogger->debug("OrderMatching copy assignment operator called");
    }
    return *this;
}

// Move assignment operator implementation
OrderMatching& OrderMatching::operator=(OrderMatching&& ordMatcher) noexcept {
    if (this != &ordMatcher) {
        clogger = std::move(ordMatcher.clogger);
        elogger = std::move(ordMatcher.elogger);
        elogger->debug("OrderMatching move assignment operator called");
    }
    return *this;
}

// Destructor implementation
OrderMatching::~OrderMatching() {
    // Only log if logger is still valid
    if (elogger) {
        elogger->debug("OrderMatching destructor called");
    }
}

// orders will be entered into orderBook
// increments orderCount atomic variable
// input: Order
// output: book - true on success fully creating a trade in orderBook.
bool OrderMatching::enterOrder(Order && ord) {
    elogger->debug("Order received! {}", ord);

    // Add order to the high-performance order book
    unsigned long orderId = orderBook->addOrder(std::move(ord));

    // Incrementing orderCount
    // This will be accessed by matchingProcess thread
	orderCount.fetch_add(1);
	orderSyncCond.notify_one();	

    elogger->debug("Order placed. Order ID: {}", orderId);
    return true;
}

// continuously running reader thread
// reads from csv, writes to orderBook vector
// signals matchingEngine, if sleeps.
bool OrderMatching::readerWriterProcess(void) {
	ExceptionRecord e;
	bool success = true;
	try {
		elogger->info("*** Reader Writer Started ...");
		auto start_time = std::chrono::high_resolution_clock::now();
		
		try{
			std::ifstream feedFile("orders.csv");
			// iterating csv till end
			unsigned long OrderIdGenerator = 0;
			for(CSVIterator loop(feedFile); loop != CSVIterator(); ++loop)
			{  
				try {
					Order ord;
					ord.orderId = OrderIdGenerator++;
					ord.trader = (*loop)[0];
					ord.stock = (*loop)[1];
					ord.quantity = stoi((*loop)[2]);
					string _side{(*loop)[3]};
					ord.side = ((_side[0] == 'B') ? TradeSide::Buy: TradeSide::Sell);
					ord.status = OrderStatus::Open;
					elogger->debug("Order parsed from csv : {}", ord);
					enterOrder(std::move(ord));
				}
				catch(std::invalid_argument& ex) {
					elogger->error("Cannot convert from iterator : {}, {}, {}, {}, Exception : {}", 
					(*loop)[0], (*loop)[1], (*loop)[2], (*loop)[3], ex.what());
					e.ex_ptr = std::current_exception();
					e.thread_name = "ReaderWriter Thread";
					e.timestamp = std::chrono::system_clock::now();
					std::lock_guard<std::mutex> gaurd(exceptMutex);
					allExceptions.push_back(e);
					success = false;
				}
				catch(std::out_of_range& ex) {
					elogger->error("Out of range while order object creation : {}, {}, {}, {}, Exception : {}", 
					(*loop)[0], (*loop)[1], (*loop)[2], (*loop)[3], ex.what());
					e.ex_ptr = std::current_exception();
					e.thread_name = "ReaderWriter Thread";
					e.timestamp = std::chrono::system_clock::now();
					std::lock_guard<std::mutex> gaurd(exceptMutex);
					allExceptions.push_back(e);
					success = false;
				}
				catch(std::runtime_error &ex){
					elogger->error("Invalid data while object creation : {}, {}, {}, {}, Exception : {}", 
					(*loop)[0], (*loop)[1], (*loop)[2], (*loop)[3], ex.what());
					e.ex_ptr = std::current_exception();
					e.thread_name = "ReaderWriter Thread";
					e.timestamp = std::chrono::system_clock::now();
					std::lock_guard<std::mutex> gaurd(exceptMutex);
					allExceptions.push_back(e);
					success = false;
				}
				if (nextOrder < orderCount) orderSyncCond.notify_one();
			}
		}
		catch (std::ios_base::failure &ex) {
			elogger->error("Can not open file orders.csv {}", ex.what());
			e.ex_ptr = std::current_exception();
			e.thread_name = "ReaderWriter Thread";
			e.timestamp = std::chrono::system_clock::now();
			std::lock_guard<std::mutex> gaurd(exceptMutex);
			allExceptions.push_back(e);
			success = false;
		}
		
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
		elogger->info("*** Reader Writer Ended ... (Execution time: {} μs)", duration);
		
		dataExausted = true;
		orderSyncCond.notify_one();
	}
	catch(const exception& ex) {
		e.ex_ptr = std::current_exception();
		e.thread_name = "ReaderWriter Thread";
		e.timestamp = std::chrono::system_clock::now();
		std::lock_guard<std::mutex> gaurd(exceptMutex);
		allExceptions.push_back(e);
		success = false;
	}
	return (success ? true: false);
}

// this function creates thread(s) and allocates work for thread
// this is a leader thread func.
// output : bool - true on successful exit.
bool OrderMatching::matchingProcess(void){
	elogger->info("**** Matching Process Started *****");
	bool success = true;
		
	while (true) {
		// Waiting till data get ready
		std::unique_lock<std::mutex> lk(orderSyncMutex);
		elogger->info("Matching process waiting for orders ...");
		orderSyncCond.wait(lk, [](){return nextOrder < orderCount;});

		while(nextOrder < orderCount) {
			try {
				// Get the order from the high-performance order book
				Order& order = orderBook->getOrder(nextOrder);
				
				// Process the order
				bool status = matcher(order);
				
				if (status) {
					elogger->info("Success: order {}", order);
				}
				else {
					elogger->info("Not Success: order {}", order);
				}
				nextOrder.fetch_add(1);
			}
			catch(const exception& ex) {
				ExceptionRecord e;
				e.ex_ptr = std::current_exception();
				e.thread_name = "Matching Thread";
				e.timestamp = std::chrono::system_clock::now();
				std::lock_guard<std::mutex> gaurd(exceptMutex);
				allExceptions.push_back(e);
				success = false;
			}
		}
		// this happens at the end-of-the-day 
		// data exausted
		if(dataExausted) break;
	}
	elogger->info("**** Matching Process Ended *****");
	return (success ? true : false);
}

// This method matches orders, and updates status as success on both the sides.
// returns true for the given order if it has enough stock on the other side.
// input  : Order
// output : bool 
// multiple worker threads consume this method
bool OrderMatching::matcher(Order& ord) {
	// Use fine-grained locking for stock queues
	auto& cs_que = ((ord.side == TradeSide::Buy) ? 
		sellMap.getQueue(ord.stock) : buyMap.getQueue(ord.stock));

	long qty = ord.quantity;
	if (not cs_que.stockQueue.empty() or cs_que.isLeftOver) {		
		if (cs_que.isLeftOver) {
			qty = qty - cs_que.leftOver.quantity;

			if (qty >= 0) {
				orderBook->updateOrderStatus(cs_que.leftOver.orderId, OrderStatus::Success);
				elogger->info("Success(!!): orderID {} ", cs_que.leftOver.orderId);
				
				cs_que.isLeftOver = false;

				if (qty == 0) {
					orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
					elogger->info("Success($$): orderID {} ", ord.orderId);
					return true;
				} 
			}
			else {
				orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
				elogger->info("Success(##): orderID {} ", ord.orderId);
				cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
				cs_que.isLeftOver = true;
				return true;
			}
		} 
		else { 
			QuantityTrader stock_in_que;
			while(qty > 0) {
				if(cs_que.stockQueue.pop(stock_in_que)){
					qty = qty - stock_in_que.quantity;
					if (qty >= 0) {
						orderBook->updateOrderStatus(stock_in_que.orderId, OrderStatus::Success);
						elogger->info("Success(!): orderID {} ", stock_in_que.orderId);
						
						cs_que.isLeftOver = false;
						if (qty == 0) {
							orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
							elogger->info("Success($): orderID {} ", ord.orderId);
							return true;
						} 
					}
					else {
						orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
						elogger->info("Success(#): orderID {} ", ord.orderId);
						cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
						cs_que.leftOver.orderId = stock_in_que.orderId;
						cs_que.isLeftOver = true;
						return true;
					}
				}
				else {
					// Use fine-grained locking for the other queue map
					auto& _que = ((ord.side == TradeSide::Buy) ?  
						buyMap.getQueue(ord.stock) : sellMap.getQueue(ord.stock));
						
					// order quantity still remains, store it in map
					QuantityTrader qt(qty, ord.orderId);
					_que.stockQueue.push(qt);
					return false;
				}
			}
		}
	}
	else {
		// if stock is first time arrived into trading, create an entry for it
		QuantityTrader qt(ord.quantity, ord.orderId);
		// Use fine-grained locking for the queue map
		auto& _que = ((ord.side == TradeSide::Buy) ? 
			buyMap.getQueue(ord.stock) : sellMap.getQueue(ord.stock));
		_que.stockQueue.push(qt);
	}	
	return false;
}

// Function to process orders for a specific stock
void OrderMatching::processStockOrders(const std::string& stock) {
    // Get all orders for this stock from the high-performance order book
    auto orders = orderBook->getOrdersByStock(stock);
    
    for (auto& orderRef : orders) {
        Order& order = orderRef.get();
        // Process each order
        matcher(order);
    }
}

// Spawns matchingProcess thread and readerWriter thread initially
// Leader thread
bool OrderMatching::orderProcess(void) {
    auto start = std::chrono::high_resolution_clock::now();
    elogger->info("**** Order Processing Started *****");

    // Group orders by stock
    std::unordered_map<std::string, std::vector<std::future<void>>> futures;

    // Extract unique stocks from the order book
    std::unordered_set<std::string> stocks;
    orderBook->forEachOrder([&stocks](const Order& order) {
        stocks.insert(order.stock);
    });

    // Create tasks for each stock
    for (const auto& stock : stocks) {
        futures[stock].push_back(threadPool->submitTask(stock));
    }

    // Wait for all tasks to complete
    for (auto& [stock, futureVec] : futures) {
        for (auto& future : futureVec) {
            future.wait();
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::microseconds>(end - start).count();
    elogger->info("**** Order Processing Ended ***** (Execution time: {} μs)", duration);
    return true;
}
