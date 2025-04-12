#define BOOST_TEST_MODULE PropertyBasedOrderMatchingTests
#include <boost/test/unit_test.hpp>
#include <random>
#include <algorithm>
#include <vector>
#include <string>
#include "OrderMatcher.hpp"

using namespace NSOrderMatching;

class PropertyTestFixture {
public:
    PropertyTestFixture() {
        // Set up the test environment
        orderMatching = std::make_unique<OrderMatching>();
    }
    
    ~PropertyTestFixture() {
        // Clean up after tests
    }

    // Random order generator for property-based testing
    Order generateRandomOrder(bool isBuy, const std::string& stock, unsigned long quantity = 0) {
        static unsigned long orderId = 0;
        static std::vector<std::string> traders = {"Trader_A", "Trader_B", "Trader_C", "Trader_D", "Trader_E"};
        static std::vector<std::string> stocks = {"Stock_X", "Stock_Y", "Stock_Z"};
        static std::random_device rd;
        static std::mt19937 gen(rd());
        
        // Distribution for traders and quantities
        std::uniform_int_distribution<> traderDist(0, traders.size() - 1);
        std::uniform_int_distribution<> quantityDist(1, 1000);
        
        Order order;
        order.orderId = orderId++;
        order.trader = traders[traderDist(gen)];
        order.stock = stock.empty() ? stocks[std::uniform_int_distribution<>(0, stocks.size() - 1)(gen)] : stock;
        order.quantity = quantity > 0 ? quantity : quantityDist(gen);
        order.side = isBuy ? TradeSide::Buy : TradeSide::Sell;
        order.status = OrderStatus::Open;
        
        return order;
    }

    // Helper function to create balanced buy and sell orders for a stock
    std::vector<Order> createBalancedOrders(const std::string& stock, unsigned long totalQuantity) {
        std::vector<Order> orders;
        
        // Create multiple buy orders
        unsigned long remainingBuy = totalQuantity;
        while (remainingBuy > 0) {
            unsigned long qty = std::min(remainingBuy, static_cast<unsigned long>(std::uniform_int_distribution<>(1, 200)(gen)));
            orders.push_back(generateRandomOrder(true, stock, qty));
            remainingBuy -= qty;
        }
        
        // Create multiple sell orders to match
        unsigned long remainingSell = totalQuantity;
        while (remainingSell > 0) {
            unsigned long qty = std::min(remainingSell, static_cast<unsigned long>(std::uniform_int_distribution<>(1, 200)(gen)));
            orders.push_back(generateRandomOrder(false, stock, qty));
            remainingSell -= qty;
        }
        
        // Shuffle orders to simulate random arrival
        std::shuffle(orders.begin(), orders.end(), gen);
        return orders;
    }
    
protected:
    std::unique_ptr<OrderMatching> orderMatching;
    std::mt19937 gen{std::random_device{}()};
};

// Property-based tests
BOOST_FIXTURE_TEST_SUITE(property_based_tests, PropertyTestFixture)

// Property 1: For balanced buy/sell orders, all orders should eventually be matched
BOOST_AUTO_TEST_CASE(test_balanced_orders_all_match) {
    // Generate 1000 balanced orders for Stock_X
    auto orders = createBalancedOrders("Stock_X", 10000);
    
    // Process all orders
    for (auto& order : orders) {
        orderMatching->enterOrder(std::move(order));
    }
    
    // Check that all orders are now in success state
    for (auto& order : orderBook) {
        BOOST_CHECK_EQUAL(static_cast<int>(order.status), static_cast<int>(OrderStatus::Success));
    }
}

// Property 2: When all sell orders are fulfilled, remaining buy orders should remain open
BOOST_AUTO_TEST_CASE(test_excess_buy_orders) {
    // Set up a known imbalance - 1500 buy quantity vs 1000 sell quantity
    const std::string testStock = "Stock_Y";
    const unsigned long buyQty = 1500;
    const unsigned long sellQty = 1000;
    
    // Create buy orders
    Order buyOrder = generateRandomOrder(true, testStock, buyQty);
    orderMatching->enterOrder(std::move(buyOrder));
    
    // Create sell orders
    Order sellOrder = generateRandomOrder(false, testStock, sellQty);
    orderMatching->enterOrder(std::move(sellOrder));
    
    // Process matching
    orderMatching->matchingProcess();
    
    // Verify properties:
    // 1. Sell order should be fully matched (Success)
    BOOST_CHECK_EQUAL(static_cast<int>(orderBook[1].status), static_cast<int>(OrderStatus::Success));
    
    // 2. Buy order should be partially matched (remain Open)
    // Note: The actual implementation might mark it as Success and create a remainder order
    // This test may need adjustment based on your exact matching algorithm
    bool foundOpenBuyOrder = false;
    for (auto& order : orderBook) {
        if (order.side == TradeSide::Buy && order.stock == testStock && 
            order.quantity == buyQty - sellQty && order.status == OrderStatus::Open) {
            foundOpenBuyOrder = true;
            break;
        }
    }
    BOOST_CHECK(foundOpenBuyOrder);
}

// Property 3: Testing invariants during various market scenarios
BOOST_AUTO_TEST_CASE(test_order_matching_invariants) {
    const std::string testStock = "Stock_Z";
    std::vector<Order> testOrders;
    
    // Scenario 1: Alternating buy/sell orders
    for (int i = 0; i < 10; i++) {
        testOrders.push_back(generateRandomOrder(i % 2 == 0, testStock, 100));
    }
    
    for (auto& order : testOrders) {
        orderMatching->enterOrder(std::move(order));
    }
    
    // Invariant: The difference between total buy and sell quantities should remain constant
    unsigned long totalBuyQty = 0, totalSellQty = 0;
    for (auto& order : orderBook) {
        if (order.stock == testStock) {
            if (order.side == TradeSide::Buy) totalBuyQty += order.quantity;
            else totalSellQty += order.quantity;
        }
    }
    
    const long initialDiff = static_cast<long>(totalBuyQty) - static_cast<long>(totalSellQty);
    
    // Process matching
    orderMatching->matchingProcess();
    
    // Recalculate quantities
    totalBuyQty = totalSellQty = 0;
    for (auto& order : orderBook) {
        if (order.stock == testStock && order.status == OrderStatus::Open) {
            if (order.side == TradeSide::Buy) totalBuyQty += order.quantity;
            else totalSellQty += order.quantity;
        }
    }
    
    const long finalDiff = static_cast<long>(totalBuyQty) - static_cast<long>(totalSellQty);
    BOOST_CHECK_EQUAL(initialDiff, finalDiff);
}

BOOST_AUTO_TEST_SUITE_END()