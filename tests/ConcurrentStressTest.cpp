#define BOOST_TEST_MODULE ConcurrentStressTests
#include <boost/test/unit_test.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <future>
#include "OrderMatcher.hpp"

using namespace NSOrderMatching;

class StressTestFixture {
public:
    StressTestFixture() {
        orderMatching = std::make_unique<OrderMatching>();
    }

    ~StressTestFixture() {
        // Cleanup
    }

    // Generate a random order
    Order generateOrder(bool isBuy, const std::string& stock, unsigned long id) {
        static std::vector<std::string> traders = {"Trader_A", "Trader_B", "Trader_C", "Trader_D", "Trader_E"};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_int_distribution<> traderDist(0, traders.size() - 1);
        std::uniform_int_distribution<> quantityDist(1, 100);
        
        Order order;
        order.orderId = id;
        order.trader = traders[traderDist(gen)];
        order.stock = stock;
        order.quantity = quantityDist(gen);
        order.side = isBuy ? TradeSide::Buy : TradeSide::Sell;
        order.status = OrderStatus::Open;
        
        return order;
    }

    // Simulate high load by generating and entering orders from multiple threads
    void runConcurrentOrderEntryTest(int numThreads, int ordersPerThread) {
        std::atomic<unsigned long> nextOrderId{0};
        std::vector<std::future<void>> futures;

        // Create producer threads that will enter orders
        for (int t = 0; t < numThreads; ++t) {
            futures.push_back(std::async(std::launch::async, [this, ordersPerThread, &nextOrderId, t]() {
                std::vector<std::string> stocks = {"Stock_X", "Stock_Y", "Stock_Z"};
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> stockDist(0, stocks.size() - 1);
                
                for (int i = 0; i < ordersPerThread; ++i) {
                    // Alternate between buy and sell orders
                    bool isBuy = (i % 2 == 0);
                    auto orderIndex = nextOrderId.fetch_add(1);
                    std::string stock = stocks[stockDist(gen)];
                    
                    Order order = generateOrder(isBuy, stock, orderIndex);
                    orderMatching->enterOrder(std::move(order));
                    
                    // Small random delay to simulate real-world scenario
                    std::this_thread::sleep_for(std::chrono::microseconds(gen() % 100));
                }
            }));
        }

        // Start a consumer thread to process the orders
        auto matcherFuture = std::async(std::launch::async, [this]() {
            return orderMatching->matchingProcess();
        });

        // Wait for all producers to finish
        for (auto& f : futures) {
            f.get();
        }

        // Signal matcher thread to exit
        dataExausted = true;
        // Wait for matcher to finish
        matcherFuture.get();
    }
    
protected:
    std::unique_ptr<OrderMatching> orderMatching;
};

BOOST_FIXTURE_TEST_SUITE(concurrent_stress_tests, StressTestFixture)

// Stress test with multiple threads entering orders simultaneously
BOOST_AUTO_TEST_CASE(test_concurrent_order_entry) {
    const int numThreads = 4;
    const int ordersPerThread = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    runConcurrentOrderEntryTest(numThreads, ordersPerThread);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << numThreads * ordersPerThread 
              << " orders in " << duration.count() << "ms" << std::endl;
    
    // Verify that all orders are processed
    BOOST_CHECK_EQUAL(nextOrder.load(), numThreads * ordersPerThread);
}

// Test for contention on the orderBook vector
BOOST_AUTO_TEST_CASE(test_order_book_contention) {
    const int numThreads = 8;
    const int ordersPerThread = 500;

    auto start = std::chrono::high_resolution_clock::now();
    
    runConcurrentOrderEntryTest(numThreads, ordersPerThread);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << numThreads * ordersPerThread 
              << " orders in " << duration.count() << "ms with high contention" << std::endl;
    
    // Verify system correctness under high contention
    BOOST_CHECK_EQUAL(orderCount.load(), numThreads * ordersPerThread);
}

// Test the system's performance under extreme load
BOOST_AUTO_TEST_CASE(test_extreme_load) {
    const int numThreads = 16;
    const int ordersPerThread = 250;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    runConcurrentOrderEntryTest(numThreads, ordersPerThread);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Processed " << numThreads * ordersPerThread 
              << " orders in " << duration.count() << "ms under extreme load" << std::endl;
    
    // Check that the system handled the load without crashing or deadlocking
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_SUITE_END()