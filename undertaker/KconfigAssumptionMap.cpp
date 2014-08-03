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
#include "PicosatCNF.h"
#include "Kconfig.h"

#include <boost/regex.hpp>

using namespace kconfig;


KconfigAssumptionMap::size_type KconfigAssumptionMap::readAssumptionsFromFile(std::istream &i) {
    std::string line;

    if (!this->_model){
        return 0;
    }
    while (std::getline(i, line)) {
        std::string sym;
        std::string val;

        if (line == "" || line == "\n" ) {
            continue;
        }
        static const boost::regex notSet_regexp("^#\\s?(CONFIG_.*) is not set$");
        static const boost::regex line_regexp("^(CONFIG_[_a-zA-Z0-9]+)=(.*)$");
        boost::smatch what;

        if (boost::regex_match(line, what, notSet_regexp)) {
            sym=what[1];
            val="n";
        } else if (boost::regex_match(line, what, line_regexp)) {
            sym=what[1];
            val=what[2];
        } else {
            if (line[0] != '#') // if we are no comment
                Logging::error("failed to parse line ", line);
            continue;
        }
        // strip of the leading 'CONFIG_'
        std::string symR = sym.substr(7, sym.length()-7);
        kconfig_symbol_type type = this->_model->getSymbolType(symR);
        std::string symM = sym + "_MODULE";

        switch(type) {
        case K_S_BOOLEAN:
            if (val == "y") {
                (*this)[sym] = true;
            } else {
                (*this)[sym] = false;
            }
            break;
        case K_S_TRISTATE:
            if (val == "y") {
                (*this)[sym] = true;
            } else if (val == "m") {
                (*this)[symM] = true;
            } else {
                (*this)[sym] = false;
                (*this)[symM] = false;
            }
            break;
        case K_S_INT:
        case K_S_HEX:
        case K_S_STRING:
            static bool warned = false;
            //TODO!!!
            if (!warned) {
                Logging::warn("S_INT, S_HEX and S_STRING cases not implemented yet");
                warned = true;
            }
            break;
        case K_S_OTHER:
        case K_S_UNKNOWN:
            break;
        default:
            //do nothing
            break;
        }
    }
    return this->size();
}
