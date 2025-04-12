#ifndef __ORDERMATCHER_HPP__
#define __ORDERMATCHER_HPP__

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>
#include <thread>
#include <vector>
#include <memory>
#include <fstream>
#include "Order.hpp"
#include "Logger.hpp"
#include "HighPerformanceOrderBook.hpp"
#include "CSVIterator.hpp"

namespace NSOrderMatching {

// No need to redefine ConcurrentStockQueue, it's already defined in Order.hpp
// Use the one from Order.hpp

class OrderMatching {
public:
    // Loggers for the order matching system
    static std::shared_ptr<spdlog::logger> elogger;
    static std::shared_ptr<spdlog::logger> clogger;

    // Thread pool for parallel order processing
    class OrderProcessorThreadPool {
    public:
        OrderProcessorThreadPool(size_t numThreads = std::thread::hardware_concurrency());
        ~OrderProcessorThreadPool();
        
        std::future<void> submitTask(const std::string& stock);
        
    private:
        void workerThread();
        std::vector<std::thread> threads_;
        std::vector<std::string> taskQueue_;
        std::mutex queueMutex_;
        std::condition_variable condition_;
        bool stop_ = false;
    };

    // Using unordered_map directly from Order.hpp since ThreadSafeMap is not defined
    static std::unordered_map<std::string, ConcurrentStockQueue> buyMap;
    static std::unordered_map<std::string, ConcurrentStockQueue> sellMap;
    
    // High-performance order book
    static std::unique_ptr<HighPerformanceOrderBook> m_orderBook;
    
    // Thread pool for parallel order processing
    static std::unique_ptr<OrderProcessorThreadPool> threadPool;

    // Constructor
    OrderMatching(); 
    
    // Copy constructor
    OrderMatching(const OrderMatching& ordMatcher);
    
    // Move constructor
    OrderMatching(OrderMatching&& ordMatcher) noexcept;
    
    // Copy assignment operator
    OrderMatching& operator=(const OrderMatching& ordMatcher);
    
    // Move assignment operator
    OrderMatching& operator=(OrderMatching&& ordMatcher) noexcept;
    
    // Destructor
    ~OrderMatching();

    // Main order processing method
    bool orderProcess(void);

    // Reader thread function
    static bool readerWriterProcess(void);

    // Order matching thread function
    static bool matchingProcess(void);

    // Process orders for a specific stock
    static void processStockOrders(const std::string& stock);

    // Process a single order
    static bool matcher(Order& ord);
    
    // Enter an order into the order book - made public for stress tests
    static bool enterOrder(Order&& ord);
};

} // namespace NSOrderMatching

#endif
