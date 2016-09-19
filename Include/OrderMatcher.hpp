#ifndef __ORDER_BOOK_HPP__
#define __ORDER_BOOK_HPP__

#include <iostream>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <vector>
#include <thread>
#include "Logger.hpp"
#include "Order.hpp"
#include "CSVIterator.hpp"

namespace NSOrderMatching {
    // All orders stored here
    class OrderMatching {
    public:
        // default constructor
        OrderMatching();
        // definitions not required for now
        OrderMatching(OrderMatching& ordMatcher){}
        OrderMatching(OrderMatching&& ordMatcher){}
        OrderMatching& operator=(OrderMatching& ordMatcher){}
        OrderMatching&& operator=(OrderMatching&& ordMatcher){}
        ~OrderMatching(){}
        
        // prodcuer thread
        static bool readerWriterProcess(void);
        // output: returns true on successfully entering a new order.
        // input : takes Order Object.
        static bool enterOrder(Order &&);
        // macthing boss thread method
		static bool matchingProcess(void);
		// matcher worker thread
		static bool matcher(Order&);
        // Server process which continously runs for orders
        bool orderProcess(void);
        // logger object for OrderBook class
        static shared_ptr<logger> elogger;
        static shared_ptr<logger> clogger;

    };
}

#endif
