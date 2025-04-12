#include "AllTests.hpp" 

// external declerations
extern std::atomic<unsigned long> orderCount;
extern std::atomic<unsigned long> nextOrder;
extern std::atomic<bool> dataExausted;  // Fixed: Changed from std::atomic_flag to std::atomic<bool>
extern std::atomic<bool> dataReady;     // Fixed: Changed from std::atomic_flag to std::atomic<bool>

extern std::mutex orderSyncMutex;
extern std::condition_variable orderSyncCond;
extern std::mutex exceptMutex;

extern std::vector<Order> orderBook;
extern std::unordered_map<std::string, ConcurrentStockQueue> buyMap;
extern std::unordered_map<std::string, ConcurrentStockQueue> sellMap;
extern std::vector<ExceptionRecord> allExceptions;
 
BOOST_AUTO_TEST_SUITE(test_order_matching)

BOOST_AUTO_TEST_CASE(test_order1) {
  OrderMatching om;
  orderBook.clear();
  auto tlogger = Logger::getLogger();
  std::ofstream myfile;

  myfile.open ("orders.csv");
  myfile << "Trader_2,Stock_X,500,Buy\n";
  myfile << "Trader_3,Stock_X,700,Buy\n";
  myfile << "Trader_1,Stock_X,1000,Sell\n";
  myfile << "Trader_4,Stock_X,200,Sell\n";
  myfile.close();

  OrderMatching::readerWriterProcess();
  BOOST_CHECK(OrderMatching::matcher(orderBook[0]) == false);
  BOOST_CHECK(OrderMatching::matcher(orderBook[1]) == false);
  BOOST_CHECK(OrderMatching::matcher(orderBook[2]) == true);
  BOOST_CHECK(orderBook[0].status == OrderStatus::Success);
  BOOST_CHECK(orderBook[2].status == OrderStatus::Success);
  BOOST_CHECK(OrderMatching::matcher(orderBook[3]) == true);
  BOOST_CHECK(orderBook[1].status == OrderStatus::Success);
  BOOST_CHECK(orderBook[3].status == OrderStatus::Success);

  tlogger->info("Passed: test_order1");
}

BOOST_AUTO_TEST_CASE(test_order2) {
  OrderMatching om;
  orderBook.clear();
  std::ofstream myfile;
  auto tlogger = Logger::getLogger();
  nextOrder = 0;
  orderCount = 0;
  myfile.open ("orders.csv");
  myfile << "Trader_2,Stock_X,1500,Sell\n";
  myfile << "Trader_3,Stock_Y,700,Buy\n";
  myfile << "Trader_5,Stock_X,1000,Buy\n";
  myfile << "Trader_1,Stock_Y,700,Sell\n";
  myfile << "Trader_1,Stock_X,500,Buy\n";
  myfile.close();

  OrderMatching::readerWriterProcess();
  BOOST_CHECK(OrderMatching::matcher(orderBook[0]) == false);
  BOOST_CHECK(OrderMatching::matcher(orderBook[1]) == false);
  BOOST_CHECK(OrderMatching::matcher(orderBook[2]) == true);

  BOOST_CHECK(orderBook[0].status == OrderStatus::Open);
  BOOST_CHECK(orderBook[2].status == OrderStatus::Success);
  BOOST_CHECK(OrderMatching::matcher(orderBook[3]) == true);
  BOOST_CHECK(orderBook[1].status == OrderStatus::Success);
  BOOST_CHECK(orderBook[3].status == OrderStatus::Success);
  
  BOOST_CHECK(OrderMatching::matcher(orderBook[4]) == true);
  BOOST_CHECK(orderBook[0].status == OrderStatus::Success);
  BOOST_CHECK(orderBook[4].status == OrderStatus::Success);

  tlogger->info("Passed: test_order2");
}

BOOST_AUTO_TEST_CASE(test_order3) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    myfile << "Trader_2,Stock_X,500,Buy\n";
    myfile << "Trader_3,Stock_X,700,Buy\n";
    myfile << "Trader_5,Stock_X,1000,Sell\n";
    myfile << "Trader_5,Stock_X,200,Sell\n";
    myfile << "Trader_1,Stock_Y,1000,Buy\n";
    myfile << "Trader_4,Stock_Y,1100,Sell\n";
    myfile << "Trader_2,Stock_Y,100,Buy\n";
    myfile << "Trader_5,Stock_Z,1000,Sell\n";
    myfile << "Trader_2,Stock_Z,200,Buy\n";
    myfile << "Trader_4,Stock_Z,800,Buy\n";
    myfile.close();

    OrderMatching::readerWriterProcess();

    BOOST_CHECK(OrderMatching::matcher(orderBook[0]) == false);
    BOOST_CHECK(OrderMatching::matcher(orderBook[1]) == false);
    BOOST_CHECK(OrderMatching::matcher(orderBook[2]) == true);
    BOOST_CHECK(orderBook[0].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[2].status == OrderStatus::Success);
    BOOST_CHECK(OrderMatching::matcher(orderBook[3]) == true);
    BOOST_CHECK(orderBook[1].status == OrderStatus::Success);
    BOOST_CHECK(OrderMatching::matcher(orderBook[4]) == true);
    BOOST_CHECK(orderBook[4].status == OrderStatus::Success);
    tlogger->info("Passed: test_order3");
}

// Add more test cases for edge scenarios
BOOST_AUTO_TEST_CASE(test_large_order) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    for (int i = 0; i < 1000; ++i) {
        myfile << "Trader_" << i << ",Stock_A," << (i + 1) * 10 << ",Buy\n";
    }
    for (int i = 0; i < 1000; ++i) {
        myfile << "Trader_" << (i + 1000) << ",Stock_A," << (i + 1) * 10 << ",Sell\n";
    }
    myfile.close();

    OrderMatching::readerWriterProcess();

    for (int i = 0; i < 1000; ++i) {
        BOOST_CHECK(OrderMatching::matcher(orderBook[i]) == true);
        BOOST_CHECK(orderBook[i].status == OrderStatus::Success);
    }
    tlogger->info("Passed: test_large_order");
}

BOOST_AUTO_TEST_CASE(test_invalid_data) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    myfile << "Trader_1,Stock_X,abc,Buy\n"; // Invalid quantity
    myfile << "Trader_2,Stock_X,500,InvalidSide\n"; // Invalid side
    myfile.close();

    bool success = OrderMatching::readerWriterProcess();
    BOOST_CHECK(success == false);
    tlogger->info("Passed: test_invalid_data");
}

BOOST_AUTO_TEST_CASE(test_no_orders) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    myfile.close();

    bool success = OrderMatching::readerWriterProcess();
    BOOST_CHECK(success == true);
    BOOST_CHECK(orderBook.empty());
    tlogger->info("Passed: test_no_orders");
}

BOOST_AUTO_TEST_CASE(test_partial_match) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    myfile << "Trader_1,Stock_X,500,Buy\n";
    myfile << "Trader_2,Stock_X,300,Sell\n";
    myfile << "Trader_3,Stock_X,200,Sell\n";
    myfile.close();

    OrderMatching::readerWriterProcess();

    BOOST_CHECK(OrderMatching::matcher(orderBook[0]) == true);
    BOOST_CHECK(orderBook[0].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[1].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[2].status == OrderStatus::Success);
    tlogger->info("Passed: test_partial_match");
}

BOOST_AUTO_TEST_CASE(test_multiple_stocks) {
    OrderMatching om;
    orderBook.clear();
    std::ofstream myfile;
    auto tlogger = Logger::getLogger();
    nextOrder = 0;
    orderCount = 0;

    myfile.open("orders.csv");
    myfile << "Trader_1,Stock_X,500,Buy\n";
    myfile << "Trader_2,Stock_Y,300,Sell\n";
    myfile << "Trader_3,Stock_Z,200,Buy\n";
    myfile << "Trader_4,Stock_X,500,Sell\n";
    myfile << "Trader_5,Stock_Y,300,Buy\n";
    myfile << "Trader_6,Stock_Z,200,Sell\n";
    myfile.close();

    OrderMatching::readerWriterProcess();

    BOOST_CHECK(OrderMatching::matcher(orderBook[0]) == true);
    BOOST_CHECK(orderBook[0].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[3].status == OrderStatus::Success);
    BOOST_CHECK(OrderMatching::matcher(orderBook[1]) == true);
    BOOST_CHECK(orderBook[1].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[4].status == OrderStatus::Success);
    BOOST_CHECK(OrderMatching::matcher(orderBook[2]) == true);
    BOOST_CHECK(orderBook[2].status == OrderStatus::Success);
    BOOST_CHECK(orderBook[5].status == OrderStatus::Success);
    tlogger->info("Passed: test_multiple_stocks");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(TestLogger)

BOOST_AUTO_TEST_CASE(test_getLogger) {
    OrderMatching om;
    auto clogger = Logger::getLogger();
    BOOST_CHECK(clogger);
    auto alogger = Logger::getAsyncLogger();
    BOOST_CHECK(alogger);
    clogger->info("Passed: test_getLogger");
}
BOOST_AUTO_TEST_SUITE_END()
