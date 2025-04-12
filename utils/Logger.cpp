#include "Logger.hpp"
#include <memory>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

using namespace spdlog;
using namespace std;

// Add the proper namespace for our Logger class
namespace NSOrderMatching {

static shared_ptr<logger> _logger = nullptr;
static shared_ptr<logger> _asyncLogger = nullptr;
static mutex _mutex;

shared_ptr<logger> Logger::getLogger() {
    lock_guard<mutex> lock(_mutex);
    if(!_logger){
        try {
            _logger = spdlog::stdout_color_mt("console");
            _logger->set_level(spdlog::level::debug); // Set to debug level
            _logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v"); // Detailed pattern
        }
        catch(const spdlog::spdlog_ex &ex){
            _logger = spdlog::get("console");
            if (_logger) {
                _logger->set_level(spdlog::level::debug); // Set to debug level
                _logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v");
            }
        }
    }
    return _logger;
}

shared_ptr<logger> Logger::getAsyncLogger() {
    lock_guard<mutex> lock(_mutex);
    if(!_asyncLogger) {
        try {
            _asyncLogger = spdlog::daily_logger_mt("daily_logger", "./logs/daily_log");
            _asyncLogger->set_level(spdlog::level::debug); // Set to debug level
            _asyncLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v"); // Detailed pattern
            _asyncLogger->flush_on(spdlog::level::debug); // Flush on debug messages too
        } 
        catch (const spdlog::spdlog_ex& ex) {
            _asyncLogger = spdlog::get("daily_logger");
            if (_asyncLogger) {
                _asyncLogger->set_level(spdlog::level::debug); // Set to debug level
                _asyncLogger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [thread %t] %v"); 
                _asyncLogger->flush_on(spdlog::level::debug);
            }
        }
    }
    return _asyncLogger;
}

} // namespace NSOrderMatching
