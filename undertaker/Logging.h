/*
 *   undertaker-logger - a thread-safe logging class
 *
 * Copyright (C) 2014 Stefan Hengelein <stefan.hengelein@fau.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _LOGGING_H_
#define _LOGGING_H_

#include <iostream>
#include <sstream>


namespace Logging {
    enum LogLevel {
        LOG_EVERYTHING = 0,
        LOG_DEBUG = 10,
        LOG_INFO = 20,
        LOG_WARNING = 30,
        LOG_ERROR = 40,
    };

    struct Logger {
        int logLevel = LogLevel::LOG_WARNING;
    };
    extern Logger logger;

    namespace {
        int getLogLevel() { return logger.logLevel; }
        void setLogLevel(int l) { logger.logLevel = l; }
    }

    // TODO gcc 4.8.1 currently has a bug and evaluates the parameters backwards...this
    // implementation is faster but prints in reverse order..
//    template <typename... Ts>
//    void l_helper(Ts...) {}

    template <typename... Ts>
    std::string buildStringFromArgs(Ts... args) {
        std::stringstream s;
#pragma GCC diagnostic ignored "-Wunused-variable"
        auto t = {(s << args, 0)...};
#pragma GCC diagnostic ignored "-Wunused-variable"
//        l_helper((s << args, 0)...);
        s << std::endl;
        return s.str();
    }

    template <typename... Ts>
    void debug(Ts... args) {
        if (logger.logLevel <= LOG_DEBUG)
            std::cout << buildStringFromArgs("D: ", std::forward<Ts>(args)...);
    }

    template <typename... Ts>
    void info(Ts... args) {
        if (logger.logLevel <= LOG_INFO)
            std::cout << buildStringFromArgs("I: ", std::forward<Ts>(args)...);
    }

    template <typename... Ts>
    void warn(Ts... args) {
        if (logger.logLevel <= LOG_WARNING)
            std::cerr << buildStringFromArgs("W: ", std::forward<Ts>(args)...);
    }

    template <typename... Ts>
    void error(Ts... args) {
        if (logger.logLevel <= LOG_ERROR)
            std::cerr << buildStringFromArgs("E: ", std::forward<Ts>(args)...);
    }
}  // namspace Logging

#endif /* _LOGGING_H_ */
