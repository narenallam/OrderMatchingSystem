
#include "Main.hpp"
// Trading system entry point.
int main() {
    auto clogger = Logger::getLogger();
    clogger->info("Trading System started ...");
    OrderMatching orderMatching;
    orderMatching.orderProcess();
    clogger->info("trading System Ended.");
    spdlog::drop_all();
    return 0;
}


