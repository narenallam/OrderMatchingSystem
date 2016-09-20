#include "OrderMatcher.hpp"
#include <chrono>

// namespace declerations
using namespace std;
using namespace spdlog;
using namespace NSOrderMatching;

// external definitions
std::atomic<unsigned long> orderCount{0};
std::atomic<unsigned long> nextOrder{0};
std::atomic_flag dataExausted{ATOMIC_FLAG_INIT};
std::atomic_flag dataReady{ATOMIC_FLAG_INIT};

std::mutex orderSyncMutex;
std::condition_variable orderSyncCond;
std::mutex exceptMutex;

std::vector<Order> orderBook;
std::unordered_map<std::string, ConcurrentStockQueue> buyMap;
std::unordered_map<std::string, ConcurrentStockQueue> sellMap;
std::vector<ExceptionRecord> allExceptions;


// logger objects for OrderBook class
// asynchronous console logger
shared_ptr<logger> OrderMatching::clogger;
// asynchronous file logger
shared_ptr<logger> OrderMatching::elogger;

OrderMatching::OrderMatching(){
	clogger = Logger::getLogger(); 
	elogger = Logger::getAsyncLogger();
	orderBook.reserve(INIT_ORDER_BOOK_SIZE);
	elogger->debug("OrderBook created with size : {}", INIT_ORDER_BOOK_SIZE);
}

// orders will be entered into orderBook
// increments orderCount atomic variable
// input: Order
// output: book - true on success fully creating a trade in orderBook.
bool OrderMatching::enterOrder(Order && ord) {
    elogger->debug("Order receieved! {}", ord);

    // orderId is vector index position,
    // we dont need a map for lateral access
    ord.orderId = orderCount;
    orderBook.emplace_back(ord);

    // Incrementing orderCount
    // This will be accessed by matchingProcess thread
	orderCount.fetch_add(1);
	orderSyncCond.notify_one();	

    elogger->debug("Order placed. {}", ord);
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
					std::lock_guard<std::mutex> gaurd(exceptMutex);
					allExceptions.push_back(e);
					success = false;
				}
				catch(std::out_of_range& ex) {
					elogger->error("Out of range while order object creation : {}, {}, {}, {}, Exception : {}", 
					(*loop)[0], (*loop)[1], (*loop)[2], (*loop)[3], ex.what());
					e.ex_ptr = std::current_exception();
					e.thread_name = "ReaderWriter Thread";
					std::lock_guard<std::mutex> gaurd(exceptMutex);
					allExceptions.push_back(e);
					success = false;
				}
				catch(std::runtime_error &ex){
					elogger->error("Invalid data while object creation : {}, {}, {}, {}, Exception : {}", 
					(*loop)[0], (*loop)[1], (*loop)[2], (*loop)[3], ex.what());
					e.ex_ptr = std::current_exception();
					e.thread_name = "ReaderWriter Thread";
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
			std::lock_guard<std::mutex> gaurd(exceptMutex);
			allExceptions.push_back(e);
			success = false;
		}
		elogger->info("*** Reader Writer Ended ...");
		dataExausted.test_and_set();
		orderSyncCond.notify_one();
	}
	catch(const exception& ex) {
		e.ex_ptr = std::current_exception();
		e.thread_name = "ReaderWriter Thread";
		std::lock_guard<std::mutex> gaurd(exceptMutex);
		allExceptions.push_back(e);
		success = false;
	}
	return (success ? true: false);
}

// this function creates thread(s) and allocates work for thread
// this is a boss thread func.
// output : bool - true on success full exit.
 bool OrderMatching::matchingProcess(void){

	elogger->info("**** Matching Process Started *****");
	bool success = true;
		// pending orders
	while (true) {

		// There is scope for parallelizing the matching proces.
		// if multiple processors exists, We can create multi-ple macthers,
		// for multiple stocks.
		// Waiting till data get ready
		std::unique_lock<std::mutex> lk(orderSyncMutex);
		elogger->info("Matching process waiting for orders ...");
		orderSyncCond.wait(lk, [](){return nextOrder < orderCount;});

		while(nextOrder < orderCount) {
			try {
				// elogger->info("Processing {}", orderBook[nextOrder]);
				bool status = matcher(orderBook[nextOrder]);
				if (status) {
					elogger->info("Success : order {}", orderBook[nextOrder]);
				}
				else {
					elogger->info("Not Success : order {}", orderBook[nextOrder]);
				}
				nextOrder.fetch_add(1);
			}			
			catch(const exception& ex) {
				ExceptionRecord e;
				e.ex_ptr = std::current_exception();
				e.thread_name = "Matching Thread";
				std::lock_guard<std::mutex> gaurd(exceptMutex);
				allExceptions.push_back(e);
				success = false;
			}
		}
		// this happens at the end-of-the-day 
		// data exausted
		if(dataExausted.test_and_set()) break;
	}
	elogger->info("**** Mathing Process Ended *****");
	return (success ? true : false);

}

// This methed macthes orders, and updates status as success on both the sides.
// returns true for the given order if it has enough stock on the other side.
// input  : Order
// output : bool 
// multiple worker threads consume this method

bool OrderMatching::matcher(Order& ord) {
	// Buyer goes to Seller
	// Seller goes to Buyer
	// elogger->info("------------------------------------------------------------------");
	// elogger->info("{} has come to {} stock: {} with qty: {} Order ID: {}", ord.trader, 
	// ((ord.side == TradeSide::Buy) ? "\'Buy\'" : "\'Sell\'"), ord.stock, ord.quantity, ord.orderId);
	auto& cs_que = ((ord.side == TradeSide::Buy) ? sellMap[ord.stock] : buyMap[ord.stock]);

	long qty = ord.quantity;
	if (not cs_que.stockQueue.empty() or cs_que.isLeftOver) {		
		if (cs_que.isLeftOver) {
			qty = qty - cs_que.leftOver.quantity;
			// elogger->info("There is left Over in the previous run. for orderId: {}, qty:{}");
			// cs_que.leftOver.orderId, cs_que.leftOver.quantity);

			if (qty >= 0) {
				orderBook[cs_que.leftOver.orderId].status = OrderStatus::Success;
				elogger->info("Success(!!): orderID {} ", cs_que.leftOver.orderId);
				
				cs_que.isLeftOver = false;

				if (qty == 0) {
					orderBook[ord.orderId].status = OrderStatus::Success;
					elogger->info("Success($$): orderID {} ", ord.orderId);
					return true;
				} 
			}
			else {
				orderBook[ord.orderId].status = OrderStatus::Success;
				elogger->info("Success(##): orderID {} ", ord.orderId);
				cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
				cs_que.isLeftOver = true;
				// elogger->info("Left Over: {}", cs_que.leftOver);
				// orderId remains same
				return true;
			}
		} 
		else { 
			// elogger->info("Tere is no leftOver, in the previous run.");
			QuantityTrader stock_in_que;
			while(qty > 0) {
				if(cs_que.stockQueue.pop(stock_in_que)){
					//elogger->info("qty : {}, Popped stock : {}", qty, stock_in_que);
					qty = qty - stock_in_que.quantity;
					if (qty >= 0) {
						orderBook[stock_in_que.orderId].status = OrderStatus::Success;
						elogger->info("Success(!): orderID {} ", stock_in_que.orderId);
						
						cs_que.isLeftOver = false;
						if (qty == 0) {
							orderBook[ord.orderId].status = OrderStatus::Success;
							elogger->info("Success($): orderID {} ", ord.orderId);
							return true;
						} 
					}
					else {
						orderBook[ord.orderId].status = OrderStatus::Success;
						elogger->info("Success(#): orderID {} ", ord.orderId);
						cs_que.leftOver.quantity = qty * (-1); // making it +ve, i.e, abs()
						cs_que.leftOver.orderId = stock_in_que.orderId;
						cs_que.isLeftOver = true;
						// orderId remains same
						return true;
					}
				}
				else {
					auto& _que = ((ord.side == TradeSide::Buy) ?  buyMap[ord.stock] : sellMap[ord.stock]);
					// order quantity still remains, store it in map
					QuantityTrader qt(qty, ord.orderId);
					_que.stockQueue.push(qt);
					// elogger->info("Stock que empty, on {} side for stock {} for {}er, adding qty:{} to {}er queue orderID {}", 
					// ((ord.side == TradeSide::Sell) ? "\'Buy\'" : "\'Sell\'"), ord.stock,
					// ((ord.side == TradeSide::Buy) ? "\'Buy\'" : "\'Sell\'"), qty,
					// ((ord.side == TradeSide::Buy) ? "\'Buy\'" : "\'Sell\'"), ord.orderId);
					return false;
				}
			}
		}
	}
	else {
		// if stock is first time arrived into trading, create an entry for it
		QuantityTrader qt(ord.quantity, ord.orderId);
		auto& _que = ((ord.side == TradeSide::Buy) ? buyMap[ord.stock] : sellMap[ord.stock]);
		_que.stockQueue.push(qt);
		// elogger->info("No {}er is available for stock: {}, so adding qty: {} to {}er queue.", 
		// ((ord.side == TradeSide::Sell) ? "\'Buy\'" : "\'Sell\'"), ord.stock, qt,
		// ((ord.side == TradeSide::Buy) ? "\'Buy\'" : "\'Sell\'"));
	}	
	return false;
}

// Spawns mathingProcess thread and readerWriter thread initially
// Boss thread
bool OrderMatching::orderProcess(void) {
    orderBook.clear();
	// hou much concurrency needed ?
	// no of logical threads = no of cores available, for maximum through put
    // elogger->info("Hardware Concurrency = {}", std::thread::hardware_concurrency());
	std::thread readerWriterThread(readerWriterProcess);
	clogger->info("data reader thread(Producer) started ...");

    auto start = std::chrono::system_clock::now();
	std::thread mathingEngineBoss(matchingProcess);
	clogger->info("matchingEngine thread(Consumer) started ...");

    readerWriterThread.join();
	clogger->info("readerWriter thread joined ...");

	mathingEngineBoss.join();
	clogger->info("matchingEngine thread joined ...");  

	auto end = std::chrono::system_clock::now();
	std::chrono::duration<double> diff = end-start;
	clogger->info("Time taken to process {} orders : {} secs", orderCount, diff.count());

	// pending exceptions from threads, if any
	for(const auto& ex: allExceptions) {
	    try {
			if (ex.ex_ptr) {
				std::rethrow_exception(ex.ex_ptr);
			}
		} catch(const std::exception& e) {
			elogger->error("Exception : {}, thread name : {}", ex.thread_name, e.what());
		}
	}
    return true;
}