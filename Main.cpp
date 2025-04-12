#include "Main.hpp"
#include <exception>
#include <spdlog/sinks/rotating_file_sink.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <ctime>
#include <iostream>
#include <string>

// Trading system entry point.
int main() {
    auto start_time = std::chrono::high_resolution_clock::now();
    std::shared_ptr<spdlog::logger> clogger;
    
    try {
        // Generate timestamp for logs
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream timestamp;
        timestamp << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d_%H-%M-%S");
        
        // Create daily log file with timestamp
        std::string log_file = "logs/daily_log_" + timestamp.str();
        clogger = spdlog::rotating_logger_mt("trading_logger", "logs/trading_system.log", 1048576 * 5, 3);
        auto daily_logger = spdlog::rotating_logger_mt("daily_logger", log_file, 1048576 * 5, 3);
        
        // Configure loggers
        clogger->set_level(spdlog::level::debug);
        daily_logger->set_level(spdlog::level::debug);
        
        // Log system startup information
        clogger->info("=========================================");
        clogger->info("Trading System started at {}", timestamp.str());
        clogger->info("=========================================");
        clogger->debug("Debug logging enabled");
        
        // Log system information
        clogger->debug("System information:");
        clogger->debug("- Thread ID: {}", std::this_thread::get_id());
        
        // Initialize order matching system
        clogger->debug("Initializing order matching system...");
        auto init_start = std::chrono::high_resolution_clock::now();
        OrderMatching orderMatching;
        auto init_end = std::chrono::high_resolution_clock::now();
        auto init_duration = std::chrono::duration_cast<std::chrono::milliseconds>(init_end - init_start).count();
        clogger->debug("Order matching system initialized in {} ms", init_duration);
        
        // Interactive debug mode
        bool continueIterating = true;
        int iterationCount = 0;
        
        while (continueIterating) {
            iterationCount++;
            
            // Process orders
            clogger->info("Starting order processing iteration #{}...", iterationCount);
            auto process_start = std::chrono::high_resolution_clock::now();
            orderMatching.orderProcess();
            auto process_end = std::chrono::high_resolution_clock::now();
            auto process_duration = std::chrono::duration_cast<std::chrono::milliseconds>(process_end - process_start).count();
            clogger->info("Order processing iteration #{} completed in {} ms", iterationCount, process_duration);
            
            // Ask if user wants to continue
            std::string response;
            std::cout << "\nContinue to iterate? (y/n): ";
            std::getline(std::cin, response);
            
            if (response.empty() || (response[0] != 'y' && response[0] != 'Y')) {
                continueIterating = false;
                clogger->info("User requested to end iteration loop after {} iterations", iterationCount);
            } else {
                clogger->info("User requested to continue iteration loop, starting iteration #{}", iterationCount + 1);
            }
        }
        
        // Log system shutdown information
        auto end_time = std::chrono::high_resolution_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        
        clogger->info("=========================================");
        clogger->info("Trading System Ended.");
        clogger->info("Total execution time: {} ms", total_duration);
        clogger->info("Total iterations run: {}", iterationCount);
        clogger->info("=========================================");
        
        daily_logger->info("Trading session summary:");
        daily_logger->info("- Session start: {}", timestamp.str());
        daily_logger->info("- Total execution time: {} ms", total_duration);
        daily_logger->info("- Total iterations run: {}", iterationCount);
        
        spdlog::drop_all();
    } catch (const std::exception &e) {
        auto error_logger = spdlog::get("trading_logger");
        if (error_logger) {
            error_logger->error("Unexpected exception: {}", e.what());
            error_logger->error("Stack trace (if available):");
            // In a real system, you might add stack trace capture here
        } else {
            std::cerr << "Unexpected exception: " << e.what() << std::endl;
        }
    } catch (...) {
        auto error_logger = spdlog::get("trading_logger");
        if (error_logger) {
            error_logger->error("Unknown exception occurred.");
            error_logger->error("Stack trace (if available):");
            // In a real system, you might add stack trace capture here
        } else {
            std::cerr << "Unknown exception occurred." << std::endl;
        }
    }
    return 0;
}


