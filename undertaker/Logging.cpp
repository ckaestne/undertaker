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

#include "Logging.h"

Logging logger; // Global log object

Logging::Logging() {
    init();
}

void Logging::log(int level, std::string input) {
    if (level >= loglevel)
        (*out) << logPrefix(level) << ": " << input << std::endl;
};

void Logging::debug(std::string input) { log(LOG_DEBUG,   input); };
void Logging::info(std::string input)  { log(LOG_INFO,    input); };
void Logging::warn(std::string input)  { log(LOG_WARNING, input); };
void Logging::error(std::string input) { log(LOG_ERROR,   input); };

void Logging::init(std::ostream &out_stream, Logging::LogLevel _loglevel,
                   Logging::LogLevel _default_loglevel) {
    out = &out_stream;
    setLogLevel(_loglevel);
    setDefaultLogLevel(_default_loglevel);
}

std::string Logging::logPrefix(int level) {
    if (level <= LOG_DEBUG) return "D";
    if (level <= LOG_INFO) return "I";
    if (level <= LOG_WARNING) return "W";
    return "E";
}

Logging & Logging::operator<< (std::ostream& (*f)(std::ostream &)) {
    std::stringstream s;
    s << f;
    if (s.str() == "\n") {
        log(actual_level, buffer.str());
        buffer.str("");
        actual_level = default_level;
    } else {
        buffer << f;
    }
    return *this;
}

Logging & Logging::operator<< (Logging& (*f)(Logging &)) {
    return (f(*this));
}

Logging& debug(Logging &l) { l.setActualLogLevel(Logging::LOG_DEBUG); return l; };
Logging& info(Logging &l)  { l.setActualLogLevel(Logging::LOG_INFO);  return l; };
Logging& warn(Logging &l)  { l.setActualLogLevel(Logging::LOG_WARNING);  return l; };
Logging& error(Logging &l) { l.setActualLogLevel(Logging::LOG_ERROR);  return l; };
