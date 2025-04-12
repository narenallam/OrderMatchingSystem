#include "OrderMatcher.hpp"
#include <chrono>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <algorithm>
#include "HighPerformanceOrderBook.hpp" // Include the header defining HighPerformanceOrderBook

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

// Global vector for backward compatibility with tests
std::vector<Order> orderBook;

// Using m_orderBook for class member to avoid collision with global orderBook
std::unique_ptr<HighPerformanceOrderBook> OrderMatching::m_orderBook = std::make_unique<HighPerformanceOrderBook>(INIT_ORDER_BOOK_SIZE);

// Using unordered_map instead of StockQueueMap
std::unordered_map<std::string, ConcurrentStockQueue> OrderMatching::buyMap;
std::unordered_map<std::string, ConcurrentStockQueue> OrderMatching::sellMap;

std::vector<NSOrderMatching::ExceptionRecord> allExceptions;

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

    // Add a copy to the global vector for test compatibility
    orderBook.push_back(ord);

    // Add order to the high-performance order book
    unsigned long orderId = m_orderBook->addOrder(std::move(ord));

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
		elogger->debug("Starting to read orders from CSV file");
		auto start_time = std::chrono::high_resolution_clock::now();
		
		try{
			std::ifstream feedFile("orders.csv");
			elogger->debug("Order CSV file opened successfully");
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
				elogger->debug("Order processed and added to order book. Current order count: {}", orderCount.load());
			}
			elogger->debug("Finished reading all orders from CSV file. Total orders processed: {}", OrderIdGenerator);
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
		elogger->debug("Reader Writer process completed. Orders read: {}, Execution time: {} μs", orderCount.load(), duration);
		
		dataExausted = true;
		orderSyncCond.notify_one();
		elogger->debug("Notified matching process that data is exhausted");
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
	elogger->debug("Matching process initialized with next order ID: {}, current order count: {}", nextOrder.load(), orderCount.load());
	bool success = true;
		
	while (true) {
		// Waiting till data get ready
		std::unique_lock<std::mutex> lk(orderSyncMutex);
		elogger->info("Matching process waiting for orders ...");
		elogger->debug("Matching process waiting for new orders. Current processed: {}, Total: {}", nextOrder.load(), orderCount.load());
		orderSyncCond.wait(lk, [](){return nextOrder < orderCount;});
		elogger->debug("Matching process woke up. New orders available: {} to process", orderCount.load() - nextOrder.load());

		while(nextOrder < orderCount) {
			try {
				// Get the order from the high-performance order book
				Order& order = m_orderBook->getOrder(nextOrder);
				elogger->debug("Processing order: {}", order);
				
				// Process the order
				bool status = matcher(order);
				
				if (status) {
					elogger->info("Success: order {}", order);
					elogger->debug("Order successfully matched: ID={}, Stock={}, Quantity={}, Side={}", 
						order.orderId, order.stock, order.quantity, (order.side == TradeSide::Buy ? "Buy" : "Sell"));
				}
				else {
					elogger->info("Not Success: order {}", order);
					elogger->debug("Order not matched completely: ID={}, Stock={}, Quantity={}, Side={}", 
						order.orderId, order.stock, order.quantity, (order.side == TradeSide::Buy ? "Buy" : "Sell"));
				}
				nextOrder.fetch_add(1);
				elogger->debug("Processed order ID: {}. Moving to next order. Progress: {}/{}", 
					order.orderId, nextOrder.load(), orderCount.load());
			}
			catch(const exception& ex) {
				ExceptionRecord e;
				e.ex_ptr = std::current_exception();
				e.thread_name = "Matching Thread";
				e.timestamp = std::chrono::system_clock::now();
				std::lock_guard<std::mutex> gaurd(exceptMutex);
				allExceptions.push_back(e);
				elogger->error("Exception in matching process: {}", ex.what());
				elogger->debug("Exception details - Thread: Matching Thread, Time: {}", 
					std::chrono::system_clock::to_time_t(e.timestamp));
				success = false;
			}
		}
		// this happens at the end-of-the-day 
		// data exausted
		if(dataExausted) {
			elogger->debug("Data exhausted, breaking matching process loop");
			break;
		}
	}
	elogger->info("**** Matching Process Ended *****");
	elogger->debug("Matching process completed. Total orders processed: {}", nextOrder.load());
	return (success ? true : false);
}

// This method matches orders, and updates status as success on both the sides.
// returns true for the given order if it has enough stock on the other side.
// input  : Order
// output : bool 
// multiple worker threads consume this method
bool OrderMatching::matcher(Order& ord) {
	auto start_time = std::chrono::high_resolution_clock::now();
	elogger->debug("Matching order: ID={}, Stock={}, Quantity={}, Side={}", 
		ord.orderId, ord.stock, ord.quantity, (ord.side == TradeSide::Buy ? "Buy" : "Sell"));
	
	// Use fine-grained locking for stock queues
	auto& cs_que = ((ord.side == TradeSide::Buy) ? 
		sellMap[ord.stock] : buyMap[ord.stock]);

	elogger->debug("Looking for matching {} orders for stock {}", 
		(ord.side == TradeSide::Buy ? "Sell" : "Buy"), ord.stock);

	long qty = ord.quantity;
	if (not cs_que.stockQueue.empty() or cs_que.isLeftOver) {
		elogger->debug("Found matching queue for stock: {}. Queue empty: {}, Has leftover: {}", 
			ord.stock, cs_que.stockQueue.empty(), cs_que.isLeftOver);
		
		if (cs_que.isLeftOver) {
			elogger->debug("Processing leftover order. Leftover order ID: {}, Quantity: {}", 
				cs_que.leftOver.orderId, cs_que.leftOver.quantity);
			qty = qty - cs_que.leftOver.quantity;
			elogger->debug("After processing leftover, remaining quantity: {}", qty);

			if (qty >= 0) {
				// Update in high-performance order book
				m_orderBook->updateOrderStatus(cs_que.leftOver.orderId, OrderStatus::Success);
				// Update in global vector for test compatibility
				if (cs_que.leftOver.orderId < orderBook.size()) {
					orderBook[cs_que.leftOver.orderId].status = OrderStatus::Success;
				}
				elogger->info("Success(!!): orderID {} ", cs_que.leftOver.orderId);
				elogger->debug("Leftover order completely filled: ID={}", cs_que.leftOver.orderId);
				
				cs_que.isLeftOver = false;

				if (qty == 0) {
					// Update in high-performance order book
					m_orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
					// Update in global vector for test compatibility
					if (ord.orderId < orderBook.size()) {
						orderBook[ord.orderId].status = OrderStatus::Success;
					}
					elogger->info("Success($$): orderID {} ", ord.orderId);
					elogger->debug("Current order completely filled: ID={}", ord.orderId);
					
					auto end_time = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
					elogger->debug("Match completed in {} μs. Result: Perfect match with leftover", duration);
					return true;
				} 
				elogger->debug("Current order partially filled. Continuing matching...");
			}
			else {
				// Update in high-performance order book
				m_orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
				// Update in global vector for test compatibility
				if (ord.orderId < orderBook.size()) {
					orderBook[ord.orderId].status = OrderStatus::Success;
				}
				elogger->info("Success(##): orderID {} ", ord.orderId);
				elogger->debug("Current order completely filled with partial leftover remaining: ID={}", ord.orderId);
				
				cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
				elogger->debug("Updated leftover quantity: {}", cs_que.leftOver.quantity);
				cs_que.isLeftOver = true;
				
				auto end_time = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
				elogger->debug("Match completed in {} μs. Result: Current order filled, leftover updated", duration);
				return true;
			}
		} 
		else { 
			elogger->debug("No leftover orders, processing from queue for stock: {}", ord.stock);
			QuantityTrader stock_in_que;
			while(qty > 0) {
				if(cs_que.stockQueue.pop(stock_in_que)){
					elogger->debug("Popped order from queue: ID={}, Quantity={}", 
						stock_in_que.orderId, stock_in_que.quantity);
					qty = qty - stock_in_que.quantity;
					elogger->debug("After matching, remaining quantity: {}", qty);
					
					if (qty >= 0) {
						// Update in high-performance order book
						m_orderBook->updateOrderStatus(stock_in_que.orderId, OrderStatus::Success);
						// Update in global vector for test compatibility
						if (stock_in_que.orderId < orderBook.size()) {
							orderBook[stock_in_que.orderId].status = OrderStatus::Success;
						}
						elogger->info("Success(!): orderID {} ", stock_in_que.orderId);
						elogger->debug("Queued order completely filled: ID={}", stock_in_que.orderId);
						
						cs_que.isLeftOver = false;
						if (qty == 0) {
							// Update in high-performance order book
							m_orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
							// Update in global vector for test compatibility
							if (ord.orderId < orderBook.size()) {
								orderBook[ord.orderId].status = OrderStatus::Success;
							}
							elogger->info("Success($): orderID {} ", ord.orderId);
							elogger->debug("Current order completely filled: ID={}", ord.orderId);
							
							auto end_time = std::chrono::high_resolution_clock::now();
							auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
							elogger->debug("Match completed in {} μs. Result: Perfect match from queue", duration);
							return true;
						}
						elogger->debug("Current order partially filled. Continuing matching...");
					}
					else {
						// Update in high-performance order book
						m_orderBook->updateOrderStatus(ord.orderId, OrderStatus::Success);
						// Update in global vector for test compatibility
						if (ord.orderId < orderBook.size()) {
							orderBook[ord.orderId].status = OrderStatus::Success;
						}
						elogger->info("Success(#): orderID {} ", ord.orderId);
						elogger->debug("Current order completely filled with partial remaining: ID={}", ord.orderId);
						
						cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
						cs_que.leftOver.orderId = stock_in_que.orderId;
						cs_que.isLeftOver = true;
						elogger->debug("Created leftover with quantity: {}, ID: {}", 
							cs_que.leftOver.quantity, cs_que.leftOver.orderId);
						
						auto end_time = std::chrono::high_resolution_clock::now();
						auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
						elogger->debug("Match completed in {} μs. Result: Current order filled, queue order partially filled", duration);
						return true;
					}
				}
				else {
					elogger->debug("Queue empty for stock: {}. Adding remaining order to opposite queue", ord.stock);
					// Use fine-grained locking for the other queue map
					auto& _que = ((ord.side == TradeSide::Buy) ?  
						buyMap[ord.stock] : sellMap[ord.stock]);
						
					// order quantity still remains, store it in map
					QuantityTrader qt(qty, ord.orderId);
					_que.stockQueue.push(qt);
					elogger->debug("Added remaining quantity {} to opposite queue. Order ID: {}", qty, ord.orderId);
					
					auto end_time = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
					elogger->debug("Match completed in {} μs. Result: Partial match, remainder queued", duration);
					return false;
				}
			}
		}
	}
	else {
		elogger->debug("No matching orders found for stock: {}. Creating new entry", ord.stock);
		// if stock is first time arrived into trading, create an entry for it
		QuantityTrader qt(ord.quantity, ord.orderId);
		// Use fine-grained locking for the queue map
		auto& _que = ((ord.side == TradeSide::Buy) ? 
			buyMap[ord.stock] : sellMap[ord.stock]);
		_que.stockQueue.push(qt);
		elogger->debug("Order added to queue: Stock={}, Quantity={}, ID={}, Side={}", 
			ord.stock, ord.quantity, ord.orderId, (ord.side == TradeSide::Buy ? "Buy" : "Sell"));
		
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
		elogger->debug("Match completed in {} μs. Result: No matching orders, queued", duration);
	}	
	return false;
}

// Function to process orders for a specific stock
void OrderMatching::processStockOrders(const std::string& stock) {
    auto start_time = std::chrono::high_resolution_clock::now();
    elogger->debug("Starting to process orders for stock: {}", stock);
    
    // Get all orders for this stock from the high-performance order book
    auto orders = m_orderBook->getOrdersByStock(stock);
    
    elogger->debug("Found {} orders for stock: {}", orders.size(), stock);
    
    int matchedCount = 0;
    int unmatchedCount = 0;
    
    for (auto& orderRef : orders) {
        Order& order = orderRef.get();
        elogger->debug("Processing {} order for stock {}: ID={}, Quantity={}", 
                     (order.side == TradeSide::Buy ? "Buy" : "Sell"),
                     order.stock, order.orderId, order.quantity);
        
        // Process each order
        bool matched = matcher(order);
        matched ? matchedCount++ : unmatchedCount++;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    
    elogger->debug("Completed processing for stock: {}. Stats: Total={}, Matched={}, Unmatched={}, Time={} μs", 
                 stock, orders.size(), matchedCount, unmatchedCount, duration);
}

// Spawns matchingProcess thread and readerWriter thread initially
// Leader thread
bool OrderMatching::orderProcess(void) {
    auto start = std::chrono::high_resolution_clock::now();
    elogger->info("**** Order Processing Started *****");
    elogger->debug("Beginning order processing with thread pool. Current order count: {}", orderCount.load());

    // Group orders by stock
    std::unordered_map<std::string, std::vector<std::future<void>>> futures;

    // Extract unique stocks from the order book
    std::unordered_set<std::string> stocks;
    elogger->debug("Extracting unique stock symbols from order book");
    m_orderBook->forEachOrder([&stocks](const Order& order) {
        stocks.insert(order.stock);
    });
    elogger->debug("Found {} unique stock symbols in order book", stocks.size());

    // Create tasks for each stock
    for (const auto& stock : stocks) {
        elogger->debug("Submitting task for stock: {}", stock);
        futures[stock].push_back(threadPool->submitTask(stock));
    }

    // Wait for all tasks to complete
    elogger->debug("Waiting for all stock processing tasks to complete");
    for (auto& [stock, futureVec] : futures) {
        for (auto& future : futureVec) {
            elogger->debug("Waiting for tasks for stock: {}", stock);
            future.wait();
            elogger->debug("Completed processing for stock: {}", stock);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    elogger->info("**** Order Processing Ended ***** (Execution time: {} μs)", duration);
    elogger->debug("Order processing complete. Processed {} orders across {} stocks. Total execution time: {} μs", 
                  orderCount.load(), stocks.size(), duration);
    return true;
}
