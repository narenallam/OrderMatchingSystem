# Order Matching system
# Problem statement:
Order Book: Write a minimalistic working application of custom order book using C++ and its standard libraries. This application will contain an order store and a matching engine. The order store is meant to record only the open orders (unmatched orders). The matching engine will match the orders based on the interest of buyers and sellers. Assume that the price is not consider. When an order is placed, the matching engine will match the order against the order(s) in the order store and notify the trader if matched.

Order will contain Trader, stock, quantity, Side (Buy or Sell).

Few use cases: 
1) Trader A places a buy order of 200 on stock S, the order is stored in the order store as open. Trader B places a sell order of 200 on stock S. Notify both the traders with success message.
2) Trader C places a sell order of 300 on stock G. Trader D places a buy order of 200 on stock G. Notify the Trader D with success message. Trader E places a Buy Order of 200 on stock G. Notify the Trader C with success message.
3) Trader W, X and Y place sell order of 200 on stock H each. Trade Z place a buy order of 600 on stock H. Trader W, X, Y and Z should be notified of success.

Write automated tests to cover all possible scenarios.

Evaluation criteria:
1. 100% functional
2. TDD (Test Driven Development)
3. Approach and Design
4. Managing concurrency (Multithreading)
5. Latency/Performance (What is the latency of your application, if 1 million Buy and Sell orders on multiple stocks are placed?)
6. Usage of Data structures.

The code should be compliable (with makefile).
Optional: Boost could be used if found adequate.

# About the Order Matching application
> Development started with below usecases

"Few use cases: 
1) Trader A places a buy order of 200 on stock S, the order is stored in the order store as open. Trader B places a sell order of 200 on stock S. Notify both the traders with success message.
2) Trader C places a sell order of 300 on stock G. Trader D places a buy order of 200 on stock G. Notify the Trader D with success message. Trader E places a Buy Order of 200 on stock G. Notify the Trader C with success message.
3) Trader W, X and Y place sell order of 200 on stock H each. Trade Z place a buy order of 600 on stock H. Trader W, X, Y and Z should be notified of success."

> I converted these usecase to unittests(refer ./tests folder), later did a lot of refactoring to achieve the results.
> used asynchronous lock-free logger for the reduction of latencies and for the real-time instrumentation.

> Designed in multi-threaded way.
There are 3 main treads
1. orderProcess - Boss threads which controls remaining 2 threads
2. matchingProcess - Wroker(Consumer) thread,  this is where the business logic goes
3. readerWriterProcess - Worker(Prodcuer) thread, this thread provides data feed for matchingProcess

OrderMatching.cpp is the main source file which contains definitions for all of the above functionalities.
> used concepts

* used std::thread for multithreading
* Used lock based concurrency primitives like std::mutex, std::condition_variable.
* Used lock-free concurrency options like Boost::lockfree::spsc_quque std::atomic_flag, std::atomics<>
* boost::lockfree::spsc_quque - is used for memory pool based memory allocation, which reduced latency to microseconds.

# Test-data generation
Test data generation - DataGenerator.py
        A python script is developed for test data generation
Description: 
    orders.csv data generator
    Generates 'random' or 'flood' type of Data or a sample orders.csv file
    random - this is default
    flood - generates data for single stock(Stock_X),
            this is for load testing.
Syntax:
    1. python DataGenerator.py [number of records] [-flood]
    2. python DataGenerator.py -sample
Usage :
    e.g,
    $ python DataGenerator.py 100000 flood 
    - above command generates orders.csv a file with 100000 records all 'Buy's of qty 1 and one 'Sell' of Stock_X
    $ python DataGenerator.py 10
    - above command generates orders.csv with random Buy and Sell of Random Quantity
    $ python DataGenerator.py sample
    - above command generates sample data of 10 orders and creats orders.csv

# Utilities

Logger.hpp - this is a wrapper for 'spdlog' - fast asynchronous logging
    console and file based logging hasbeen implemented, console based logging is used in the application for time being.
Boost::test - for unit testing
CSVIterator.hpp - for csv reading

# How to run the apllication

Prerequisites:

    boost libraries
    python
    platforms : Linux(Ubuntu) or Mac OS X

Process:

    > cd OrderMatching
    > make clean && make
    > python DataGenerator.py -sample
    > ./run

# running tests

    > cd OrderMatching/tests
    > make clean && make
    > ./runtests

# logs
> logs can be found in .logs folder

# Performance and Benchmarking
Various performance and load tests are conducted.
When there are a million orders on a single stock, that will become the worst case behaviour of the application.

    NarenMacBook% python DataGenerator.py 1000000 -flood
    Success: orders.csv generated! with 1000000 single stock records.
    NarenMacBook% ./run
    [13:18:46:738 +05:30][console][info][thread 18365856221335225246]: Trading System started ...
    [13:18:46:740 +05:30][console][info][thread 18365856221335225246]: data reader thread(Producer) started ...
    [13:18:46:740 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread(Consumer) started ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: readerWriter thread joined ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread joined ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: Time taken to process 1000001 orders : 5.20522 secs
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: trading System Ended.
    NarenMacBook% python DataGenerator.py 1000000
    Success: orders.csv generated! with 1000000 random records.
    NarenMacBook% ./run
    [13:19:33:817 +05:30][console][info][thread 18365856221335225246]: Trading System started ...
    [13:19:33:818 +05:30][console][info][thread 18365856221335225246]: data reader thread(Producer) started ...
    [13:19:33:818 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread(Consumer) started ...
    [13:19:39:075 +05:30][console][info][thread 18365856221335225246]: readerWriter thread joined ...
    [13:19:39:075 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread joined ...
    [13:19:39:075 +05:30][console][info][thread 18365856221335225246]: Time taken to process 1000000 orders : 5.25691 secs
    [13:19:39:075 +05:30][console][info][thread 18365856221335225246]: trading System Ended.
    NarenMacBook%

# Conclusion
Order Matching Application is 

1. 100% functional
> aplication is 100% functional

2. TDD (Test Driven Development)

> Development started with Boost::Test unit tests

3. Approach and Design

> Producer - Consumer and Boss-Worker threading models
    The design depends on which runs faster(Producer or Consumer).
    In a typical trading system producer is always faster than the consumer
    if procuder is taking X time and consumer is taking Y time to process, 
    we create X/Y Producer threads, if those many processors are available.
    98% apllication logic is designed using lock-free concurrency.
    memory pooling is used to reduce the memory allocation costs(boost::lockfree::spsc_queue)
> scalablity: as we did lock-free appraoch application is highly-scalable
> singleton designpattern is used for Logger objects

4. Managing concurrency (Multithreading)

> used lock-free/ lock-based primitives to manage concurrency

5. Latency/Performance (What is the latency of your application, if 1 million Buy and Sell orders on multiple stocks are placed?)
    Time taken to process 1000000 orders : 5.25691 secs [random stocks]
    Time taken to process 1000001 orders : 5.20522 secs [single stock]

6. Usage of Data structures.
> used std::unordered_map, std::vector
> used boost::lockfree::spsc_queue

