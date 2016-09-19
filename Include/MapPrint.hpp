#include "Order.hpp"
using namespace NSOrderMatching;
// utility method for map printing
// input: buyMap and sellMap
void printMaps(unordered_map<std::string, ConcurrentStockQueue>& buyMap, 
              unordered_map<std::string, ConcurrentStockQueue>& sellMap) {/*
    auto elogger = spdlog::get("console");

    for (const auto& pair: buyMap ){
        elogger->debug("----BUY MAP-------- {} -> ", pair.first);
        while(not pair.second.stockQueue.empty()) {
            elogger->debug("{}", pair.second.stockQueue.front());
            pair.second.stockQueue.pop();
        }
        elogger->debug("------------ {} -> ");

    }
     for (const auto& pair: sellMap ){
        elogger->debug("----SELL MAP-------- {} -> ", pair.first);

        while(not pair.second.stockQueue.empty()) {
            elogger->debug("{}", pair.second.stockQueue.front());
            pair.second.stockQueue.pop();
        }
        elogger->debug("------------ {} -> ");

    }*/
}