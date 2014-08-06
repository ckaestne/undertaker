/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


class Logging {
public:
    Logging();

    enum LogLevel {
        LOG_EVERYTHING = 0,
        LOG_DEBUG = 10,
        LOG_INFO = 20,
        LOG_WARNING = 30,
        LOG_ERROR = 40,
    };
    void log(int level, std::string input);

    void debug(std::string input);
    void info(std::string input);
    void warn(std::string input);
    void error(std::string input);

    void setLogLevel(int l) { loglevel = l; };
    void setDefaultLogLevel(int l) { default_level = l; };
    void setActualLogLevel(int l) { actual_level = l; };
    int getLogLevel() { return loglevel; }

    void init(std::ostream &out_stream=std::cout,
              std::ostream &error_stream=std::cerr,
              LogLevel loglevel=LOG_WARNING,
              LogLevel default_loglevel=LOG_WARNING);

    /* Catchall for all other stream operator<< arguments */
    template<typename T>
    Logging & operator<< (const T &t) {
        buffer << t;
        return *this;
    };

    Logging & operator<< (std::ostream& (*f)(std::ostream &));
    Logging & operator<< (Logging& (*f)(Logging &));

private:
    std::string logPrefix(int level);
    int loglevel, default_level, actual_level;
    std::ostream *out;
    std::ostream *err;
    std::stringstream buffer;
};

extern Logging logger;

Logging& debug(Logging &l);
Logging& info(Logging &l);
Logging& warn(Logging &l);
Logging& error(Logging &l);


#endif /* _LOGGING_H_ */
