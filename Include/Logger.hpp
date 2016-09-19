#ifndef __LOGGER_HPP__
#define __LOGGER_HPP__

#include <memory>
#include <iostream>

#include "spdlog/spdlog.h"
#include "spdlog/logger.h"
#include <spdlog/fmt/ostr.h>

using namespace std;
using namespace spdlog;

class Logger {
    public : 
        static shared_ptr<logger> getLogger();
        static shared_ptr<logger> getAsyncLogger();
    private:
        Logger(){} // no object should get created directly
        ~Logger(){} // cannot be inherited
        static shared_ptr<logger> _logger;
        static shared_ptr<logger> _asyncLogger;
};

#endif
