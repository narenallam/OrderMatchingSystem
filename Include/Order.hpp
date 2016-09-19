#ifndef __ORDER_HPP__
#define __ORDER_HPP__

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <vector>
#include <tuple>
#include <queue>
#include <boost/lockfree/queue.hpp>
#include <boost/lockfree/spsc_queue.hpp>

namespace NSOrderMatching {
        
    // Constants 
    const long int MAX_STRING_LENGTH = 32;
    const long int MAX_QUANTITY = 10000;
    const long int INIT_ORDER_BOOK_SIZE = 1000000; // 10 million
    const long int INIT_STOCK_QUEUE_SIZE = 1000000; // 10 million

    // enumerations
    enum class TradeSide {Buy, Sell};
    enum class OrderStatus {Open, Success};

    struct Order {
    public:
        // default constructor 
        Order(){}

        // Parameterized Constructor
        Order(unsigned long _ordId, std::string _trader, std::string _stock, TradeSide _side, unsigned long _qty): 
              orderId{_ordId}, trader{_trader}, stock{_stock}, 
              side{_side}, quantity{_qty}, status{OrderStatus::Open}{}

        // copy constructor 
        Order(Order & ordr):
            orderId{ordr.orderId},
            trader{ordr.trader},
            stock{ordr.stock},
            side{ordr.side},
            quantity{ordr.quantity},
            status{ordr.status}{}
            
        // trivial move constructor
        Order(Order && ordr) = default;

        // trivial move assigment
        Order& operator=(Order && ordr) = default;

        // trivial destructor
        ~Order() = default;

        /* Overloading ostream for logging and printing purpose. */
        friend std::ostream& operator<<(std::ostream &output, const Order &ord) { 
            output << ord.orderId
                    << ", " << ord.trader 
                    << ", " << ord.stock
                    << ", "  << ((ord.side == TradeSide::Buy) ? "Buy" : "Sell")
                    << ", " << ord.quantity
                    << ", " << ((ord.status == OrderStatus::Open) ? "Open": "Success");
                
         return output;            
        }

        /* Order Properties */
        unsigned long orderId;
        std::string trader;
        std::string stock;
        TradeSide side;
        unsigned long quantity;
        OrderStatus status;
    };

    // Quantity OrderId struct for matching dictionaries.
    // to become boost::lockfree, copy constructor must be defined, 
    // assignment and destructor must be trivial.
    // strcut intended to be POD.
    struct QuantityTrader {
        // properties
        unsigned long quantity;
        unsigned long orderId;

        // constructor
        QuantityTrader(){}
        // para constructor
        QuantityTrader(unsigned long _qty, unsigned long _ordId):
                       quantity{_qty}, orderId(_ordId){}
        // copy constructor
        QuantityTrader(const QuantityTrader &qtObj):
                       quantity{qtObj.quantity}, orderId(qtObj.orderId){}
        
        // assignment
        QuantityTrader& operator=(const QuantityTrader &qtObj) = default;
        
        // move cosntructor
        QuantityTrader(QuantityTrader &&qtObj) = default;

        // trivial move assignment
        QuantityTrader& operator=(QuantityTrader &&qtObj) = default;
        
        // trivial destructor
        ~QuantityTrader() = default;
                /* Overloading ostream for logging and printing purpose. */
        friend std::ostream& operator<<(std::ostream &output, const QuantityTrader &ord) { 
            output << "{"<< ord.orderId << ", " << ord.quantity << "}";
         return output;            
        }
    };

    struct ConcurrentStockQueue {
        //boost::lockfree::queue<QuantityTrader> stockQueue{INIT_STOCK_QUEUE_SIZE};
        //std::queue<QuantityTrader> stockQueue;
        boost::lockfree::spsc_queue<QuantityTrader> stockQueue{1000001};
        bool isLeftOver{false};
        QuantityTrader leftOver{0,0}; // leftover quantity of stock in the last run
    };
}

// START - Global data shared by all threads ------
// all orders are stored here
// initialized with INIT_ORDER_BOOK_SIZE
extern std::vector<NSOrderMatching::Order> orderBook;

// buying data
// key - stock name 
// value - boost lock free queue of QuantityTrader objects
// each stock maintains a lock-free queue, easy for scaling (multiple worker threads)
extern std::unordered_map<std::string, NSOrderMatching::ConcurrentStockQueue> buyMap;

// selling data
// key - stock name 
// value - boost lock free queue of QuantityTrader objects
// each stock maintains a lock-free queue, easy for scaling (multiple worker threads)
extern std::unordered_map<std::string, NSOrderMatching::ConcurrentStockQueue> sellMap;

// sync objects. Both threads will be synced using these objects.
extern std::atomic_flag dataReady;
extern std::mutex orderSyncMutex;
extern std::condition_variable orderSyncCond;
extern std::atomic_flag dataExausted;

// orders so far, this will be accessed by mathing engine thread
extern std::atomic<unsigned long> orderCount;
extern std::atomic<unsigned long> nextOrder;

// small utility struct for exception-handling
struct ExceptionRecord{
    const char* thread_name;
    std::exception_ptr ex_ptr;
};

// for multi-threaded exception handling
extern std::vector<ExceptionRecord> allExceptions;
extern std::mutex exceptMutex;

// END - Global data shared by all threads ---------

#endif