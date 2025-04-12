#ifndef __ORDER_BOOK_HPP__
#define __ORDER_BOOK_HPP__

#include <iostream>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <thread>
#include <future>
#include "Logger.hpp"
#include "Order.hpp"
#include "CSVIterator.hpp"
#include "HighPerformanceOrderBook.hpp"

namespace NSOrderMatching {
    // All orders stored here
    class OrderMatching {
    public:
        // default constructor
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
        
        // prodcuer thread
        static bool readerWriterProcess(void);
        // output: returns true on successfully entering a new order.
        // input : takes Order Object.
        static bool enterOrder(Order &&);
        // matching leader thread method
		static bool matchingProcess(void);
		// matcher worker thread
		static bool matcher(Order&);
        // Process orders for a specific stock
        static void processStockOrders(const std::string& stock);
        // Server process which continuously runs for orders
        bool orderProcess(void);
        // logger object for OrderBook class
        static shared_ptr<logger> elogger;
        static shared_ptr<logger> clogger;
        
    private:
        // High-performance order book
        static std::unique_ptr<HighPerformanceOrderBook> orderBook;
        
        // Lock-free per-stock queue maps with fine-grained locking
        struct StockQueueMap {
            std::unordered_map<std::string, ConcurrentStockQueue> queueMap;
            std::mutex mutex;
            
            ConcurrentStockQueue& getQueue(const std::string& stock) {
                std::lock_guard<std::mutex> lock(mutex);
                return queueMap[stock];
            }
        };
        
        static StockQueueMap buyMap;
        static StockQueueMap sellMap;
        
        // Worker thread pool for processing orders by stock
        class OrderProcessorThreadPool {
        public:
            OrderProcessorThreadPool(size_t numThreads = std::thread::hardware_concurrency());
            ~OrderProcessorThreadPool();
            
            // Submit a task to process orders for a stock
            std::future<void> submitTask(const std::string& stock);
            
        private:
            std::vector<std::thread> threads_;
            std::vector<std::string> taskQueue_;
            std::mutex queueMutex_;
            std::condition_variable condition_;
            std::atomic<bool> stop_{false};
            
            void workerThread();
        };
        
        static std::unique_ptr<OrderProcessorThreadPool> threadPool;
    };
}

#endif
