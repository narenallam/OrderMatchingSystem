# Order Matching System

## What's New - 2025 Update

We've implemented several key improvements to enhance the Order Matching System:

1. **Memory Management & Modern C++ Features**
   - Implemented proper Rule of Five (copy/move constructors, assignments, destructor) for OrderMatching class
   - Enhanced move semantics for Order class string members for better efficiency
   - Added explicit move operations to reduce unnecessary object copying

2. **Thread Safety Enhancements**
   - Replaced `std::atomic_flag` with more readable `std::atomic<bool>` for improved code clarity
   - Fixed synchronization mechanisms with cleaner atomic operations
   - Made queue size configurable through the Configuration structure

3. **Exception Handling Improvements**
   - Enhanced ExceptionRecord structure with std::string and timestamps
   - Added consistent timestamp tracking for all exceptions
   - Improved exception context for better debugging

4. **Performance Monitoring**
   - Added high-precision timing using `std::chrono::high_resolution_clock`
   - Implemented execution time logs to identify performance bottlenecks
   - Enhanced debug logging for critical operations

5. **Configuration Management**
   - Created centralized Configuration structure for system constants
   - Made queue sizes and other parameters configurable for different environments
   - Improved code maintainability by eliminating hardcoded values

## About the Order Matching Application

* Development started with below use cases:

#### Use cases:

        1) Trader A places a buy order of 200 on stock S, the order is stored in the order store as open. 
        Trader B places a sell order of 200 on stock S. Notify both the traders with success message.

        2) Trader C places a sell order of 300 on stock G. 
        Trader D places a buy order of 200 on stock G. Notify the Trader D with success message. 
        Trader E places a Buy Order of 200 on stock G. Notify the Trader C with success message.

        3) Trader W, X and Y place sell order of 200 on stock H each. 
        Trade Z place a buy order of 600 on stock H. 
        Trader W, X, Y and Z should be notified of success.

* I converted these use cases to unittests(refer ./tests folder), later did a lot of refactoring to achieve the results.
* used asynchronous lock-free logger for the reduction of latencies and for the real-time instrumentation.

## Design and Approach

    * There are two unordered_maps one for sell and one for buy.
    * Each map contains {stock: orderqueue} associations.
    * orderqueue is memory-pooled container, faster pop() and push() operations.
    * When a new buy order arrives, matcher deducts all sell orders from the  orderqueue, 
      and updates the status to 'Success'
    * When a new sell order arrives, matcher deducts all buy orders from the  orderqueue,
      and updates the status to 'Success'.
              
    Concurrency:
        Application is designed in multi-threaded way.
        but the memory-pooled concept of boost::lockfree::spsc_queue is utilized.
              
    Scalability:
        
        As, the data structures are lock-free, and there is scope for high-scalability.
        
## Used concepts

    Multithreading : 
        * used std::thread for multithreading
        
    Synchronization :
        * Used lock based concurrency primitives like std::mutex, std::condition_variable.
        * boost::lockfree::spsc_quque - is used for memory pool based memory allocation, which reduced latency to microseconds.
        
    Data Structures:
        * std::vector # for multi-threaded exception handling
        * boost::lockfree::spsc_queue # for stock matching
        
    Designpatterns:
        * singleton designpattern is used for Logger objects
    
    Memory Management:
        * Modern C++ Rule of Five implemented for proper resource management
        * Enhanced move semantics for string members to reduce copying
        * Smart pointers for better memory safety

## Test-data generation

    Test data generation - DataGenerator.py:
        A python script is developed for test data generation.
        User can create, multiple test data files for functional and load testing
    Description: 
        orders.csv data generator
    Syntax:
        1. python DataGenerator.py [number of records] [-flood]
        2. python DataGenerator.py -sample
    Usage :
        e.g,
        - above command generates sample data of 10 orders and creates orders.csv

## Utilities

    Logger.hpp - this is a wrapper for 'spdlog' - fast asynchronous logging
        console and file based logging has been implemented, console based logging is used in the application for time being.
    Boost::test - for unit testing
    CSVIterator.hpp - for csv reading

## How to run the application

### Prerequisites:

    boost libraries
    python
    platforms : Linux(Ubuntu) or Mac OS X

### Process:

    > cd OrderMatching
    > make clean && make
    > python DataGenerator.py -sample
    > ./run

### Typical Output for a sample run looks like below:
####  Orders.csv
    Trader_2,Stock_X,500,Buy
    Trader_3,Stock_X,700,Buy
    Trader_5,Stock_X,1000,Sell
    Trader_5,Stock_X,200,Sell
    Trader_1,Stock_Y,1000,Buy
    Trader_4,Stock_Y,1100,Sell
    Trader_2,Stock_Y,100,Buy
    Trader_5,Stock_Z,1000,Sell
    Trader_2,Stock_Z,200,Buy
    Trader_4,Stock_Z,800,Buy
####  logfile content:
    [23:32:20:082 +05:30][async_file_logger][info][thread 14316483420521963862]: *** Reader Writer Started ...
    [23:32:20:082 +05:30][async_file_logger][info][thread 17003373142924278091]: **** Matching Process Started *****
    [23:32:20:082 +05:30][async_file_logger][info][thread 17003373142924278091]: Matching process waiting for orders ...
    [23:32:20:083 +05:30][async_file_logger][info][thread 14316483420521963862]: *** Reader Writer Ended ...
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Not Success : order 0, Trader_2, Stock_X, Buy, 500, Open
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Not Success : order 1, Trader_3, Stock_X, Buy, 700, Open
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(!): orderID 0 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(#): orderID 2 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success : order 2, Trader_5, Stock_X, Sell, 1000, Success
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(!!): orderID 1 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success($$): orderID 3 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success : order 3, Trader_5, Stock_X, Sell, 200, Success
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Not Success : order 4, Trader_1, Stock_Y, Buy, 1000, Open
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(!): orderID 4 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Not Success : order 5, Trader_4, Stock_Y, Sell, 1100, Open
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(!): orderID 5 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success($): orderID 6 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success : order 6, Trader_2, Stock_Y, Buy, 100, Success
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Not Success : order 7, Trader_5, Stock_Z, Sell, 1000, Open
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(#): orderID 8 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success : order 8, Trader_2, Stock_Z, Buy, 200, Success
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success(!!): orderID 7 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success($$): orderID 9 
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: Success : order 9, Trader_4, Stock_Z, Buy, 800, Success
    [23:32:20:083 +05:30][async_file_logger][info][thread 17003373142924278091]: **** Mathing Process Ended *****

## High-Level Architecture Diagram

Below is a high-level architecture diagram to visualize the threading model and data flow:

```
[DataGenerator] --> [OrderProcessor (leader thread)] --> [MatchingEngine (Worker Thread)]
```

- **DataGenerator**: Generates test data for the system.
- **OrderProcessor**: Reads and processes orders.
- **MatchingEngine**: Matches buy and sell orders.

## Running tests

    > cd OrderMatching/tests
    > make clean && make
    > ./runtests

## Logs
    logs can be found in ./logs folder

## Scope for enhancements:

    1. Further Parallelization:
        A single stock type processing is independent of the other. So,
        we can create more worker threads (if more cores available), and share work among the workers.
    
    2. Observer Pattern:
        Implement a proper Observer pattern for more structured trade notifications.
    
    3. Configuration Management:
        Move hardcoded constants to an external configuration system.
    
    4. Performance Optimization:
        Continue improving data structures and algorithms for high-frequency trading.

## Performance and Benchmarking

    Various performance and load tests are conducted.
    When there are a million orders on a single stock, that will be the worst case behavior of the application.
    We can reduce this by prallelizing the 'Success' updating task.
    The below benchmarks include log processing.

    NarenMacBook% python DataGenerator.py 1000000 -flood
    Success: orders.csv generated! with 1000000 single stock records.
    NarenMacBook% ./run
    [13:18:46:738 +05:30][console][info][thread 18365856221335225246]: Trading System started ...
    [13:18:46:740 +05:30][console][info][thread 18365856221335225246]: data reader thread(Producer) started ...
    [13:18:46:740 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread(Consumer) started ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: readerWriter thread joined ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: matchingEngine thread joined ...
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: Time taken to process 1000001 orders : 4.22045 secs
    [13:18:51:945 +05:30][console][info][thread 18365856221335225246]: trading System Ended.
    NarenMacBook% python DataGenerator.py 1000000
    Success: orders.csv generated! with 1000000 random records.
    NarenMacBook% ./run
    [13:19:33:817 +05:30][console][info][thread 18365856221335225246]: Trading System started ...
    [13:19:33:818 +05:30][console][info][thread 18365856221335225246]: data reader thread(Producer) started ...
    NarenMacBook%

# Conclusion

### Order Matching Application satisfies below criteria, 

## 1) 100% functional

    Application is 100% functional, as it satisfies all the use-cases.

## 2) TDD (Test Driven Development)

    *** No errors detected

## 3) Approach and Design: 
     Refer Approach and Design
   
## 5) Managing concurrency (Multithreading)
     Refer Approach and Design
     Enhanced with better atomic types for readability and safety

## 6) Latency/Performance 
     Performance tracking now implemented with high-precision timing
     Near real-time processing: 4.22 seconds for 1 million orders

## 7) Usage of Data Structures:
     Used boost::lockfree::spsc_queue with configurable size
     Enhanced memory management with proper Rule of Five
