#include "Logger.hpp"
#include <memory>
#include <mutex>
using namespace spdlog;
using namespace std;

shared_ptr<logger> Logger::_logger=nullptr;
shared_ptr<logger> Logger::_asyncLogger=nullptr;

shared_ptr<logger> Logger::getLogger() {
    try {
        // Singleton - double check locking
        if (not _logger) {
            std::mutex sync_threads;
            std::lock_guard<std::mutex> gaurd(sync_threads);
            if (not _logger) {
                // Mutli-threaded Logger object creation
                _logger = stdout_logger_mt("console", true);
                spdlog::set_level(level::info);
                spdlog::set_async_mode(4096);
                //spdlog::flush_on(spdlog::level::info);
                spdlog::set_pattern("[%H:%M:%S:%e %z][%n][%l][thread %t]: %v");
            }
        }
    }
    catch (const spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return nullptr;
    }
    return _logger;
}
shared_ptr<logger> Logger::getAsyncLogger() {
    try {
        // Singleton - double check locking
        if (not _asyncLogger) {
            std::mutex sync_threads;
            std::lock_guard<std::mutex> gaurd(sync_threads);
            if (not _asyncLogger) {
                spdlog::set_level(level::info);
                // multi-threaded async file Logger object creation
                size_t q_size = 4096; //queue size must be power of 2
                spdlog::set_async_mode(q_size);
                // at 1:30 AM a new log file gets created
                _asyncLogger = spdlog::daily_logger_mt("async_file_logger", "logs/daily_log", 1, 30);
                set_level(level::info);
                spdlog::set_pattern("[%H:%M:%S:%e %z][%n][%l][thread %t]: %v");
            }
        }
    }
    catch (const spdlog_ex& ex) {
        std::cout << "Log init failed: " << ex.what() << std::endl;
        return nullptr;
    }
    return _asyncLogger;
}
