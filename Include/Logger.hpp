#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

// Remove the redefinition of SPDLOG_FMT_EXTERNAL since it's already defined by Conan's fmt
// #define SPDLOG_FMT_EXTERNAL 0

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>

namespace NSOrderMatching {
    // Removed the duplicate ExceptionRecord struct
    
	class Logger {

	public:

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;
		
		Logger() = delete; // Non constructible
		
		static std::shared_ptr<spdlog::logger> getLogger();
		// async file logger example
		static std::shared_ptr<spdlog::logger> getAsyncLogger();

	};
}

#endif
