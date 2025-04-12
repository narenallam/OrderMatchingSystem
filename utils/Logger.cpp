#define SPDLOG_FMT_EXTERNAL 0
#include "Logger.hpp"
#include <memory>
#include <mutex>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/daily_file_sink.h>

using namespace spdlog;
using namespace std;

shared_ptr<logger> Logger::_logger = nullptr;
shared_ptr<logger> Logger::_asyncLogger = nullptr;
mutex Logger::_mutex;

shared_ptr<logger> Logger::getLogger() {
    lock_guard<mutex> lock(_mutex);
    if(!_logger){
        try {
            _logger = spdlog::stdout_color_mt("console");
        }
        catch(const spdlog::spdlog_ex &ex){
            _logger = spdlog::get("console");
        }
    }
    return _logger;
}

shared_ptr<logger> Logger::getAsyncLogger() {
    lock_guard<mutex> lock(_mutex);
    if(!_asyncLogger) {
        try {
            _asyncLogger = spdlog::daily_logger_mt("daily_logger", "./logs/daily_log");
        } 
        catch (const spdlog::spdlog_ex& ex) {
            _asyncLogger = spdlog::get("daily_logger");
        }
    }
    return _asyncLogger;
}
