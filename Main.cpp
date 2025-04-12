#include "Main.hpp"
#include <exception>
#include <spdlog/sinks/rotating_file_sink.h>

// Trading system entry point.
int main() {
    try {
        auto clogger = spdlog::rotating_logger_mt("trading_logger", "logs/trading_system.log", 1048576 * 5, 3);
        clogger->set_level(spdlog::level::info);
        clogger->info("Trading System started ...");

        OrderMatching orderMatching;
        orderMatching.orderProcess();

        clogger->info("Trading System Ended.");
        spdlog::drop_all();
    } catch (const std::exception &e) {
        auto error_logger = spdlog::get("trading_logger");
        if (error_logger) {
            error_logger->error("Unexpected exception: {}", e.what());
        } else {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
        }
    } catch (...) {
        auto error_logger = spdlog::get("trading_logger");
        if (error_logger) {
            error_logger->error("Unknown exception occurred.");
        } else {
            std::cerr << "Unknown exception occurred." << std::endl;
        }
    }
    return 0;
}


