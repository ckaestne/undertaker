/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "KconfigAssumptionMap.h"
#include "Logging.h"

#ifndef LKC_DIRECT_LINK
#define LKC_DIRECT_LINK
#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "lkc.h"
#pragma GCC diagnostic warning "-Wunused-parameter"

#include <boost/regex.hpp>

#include <sstream>

using namespace kconfig;

KconfigAssumptionMap::KconfigAssumptionMap(CNF *model) : _model(model) {}

KconfigAssumptionMap::size_type KconfigAssumptionMap::readAssumptionsFromFile(std::istream &i) {
    std::string line;

    if (!this->_model){
        return 0;
    }
    while (std::getline(i,line)) {
        std::stringstream l(line);
        std::string sym;
        std::string val;

        if (line == "" || line == "\n" ) {
            continue;
        }
        static const boost::regex notSet_regexp("^#\\s?(CONFIG_.*) is not set$");
        static const boost::regex line_regexp("^(CONFIG_[_a-zA-Z0-9]+)=(.*)$");
        boost::match_results<std::string::const_iterator> what;

        if (boost::regex_match(line, what, notSet_regexp)) {
            sym=what[1];
            val="n";
        } else if (boost::regex_match(line, what, line_regexp)) {
            sym=what[1];
            val=what[2];
        } else {
            if (line[0] != '#') // if we are no comment
                logger << error << "failed to parse line " << line << std::endl;
            continue;
        }
        // strip of the leading 'CONFIG_'
        std::string symR = sym.substr(7, sym.length()-7);
        enum symbol_type type = (enum symbol_type) this->_model->getSymbolType(symR);
        std::string symM = sym + "_MODULE";

        switch(type) {
        case S_BOOLEAN:
            if (val == "y") {
                (*this)[sym] = true;
            } else {
                (*this)[sym] = false;
            }
            break;
        case S_TRISTATE:
            if (val == "y") {
                (*this)[sym] = true;
            } else if (val == "m") {
                (*this)[symM] = true;
            } else {
                (*this)[sym] = false;
                (*this)[symM] = false;
            }
            break;
        case S_INT:
        case S_HEX:
        case S_STRING:
            static bool warned = false;
            //TODO!!!
            if (!warned) {
                logger << warn << "S_INT, S_HEX and S_STRING cases not implemented yet"
                       << std::endl;
                warned = true;
            }
            break;
        case S_OTHER:
        case S_UNKNOWN:
            break;
        default:
            //do nothing
            break;
        }
    }
    return this->size();
}
