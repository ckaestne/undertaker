/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
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

#include "ConditionalBlock.h"
#include "CnfConfigurationModel.h"
#include "KconfigWhitelist.h"
#include "StringJoiner.h"
#include "RsfReader.h"
#include "Logging.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>


CnfConfigurationModel::CnfConfigurationModel(const char *filename) {
    const StringList *configuration_space_regex = NULL;
    boost::filesystem::path filepath(filename);

    _name = boost::filesystem::basename(filepath);
    _model_stream = new std::ifstream(filename);

    _cnf = new kconfig::PicosatCNF();
    _cnf->readFromFile(*_model_stream);
    configuration_space_regex = _cnf->getMetaValue("CONFIGURATION_SPACE_REGEX");

    if (configuration_space_regex != NULL && configuration_space_regex->size() > 0) {
        logger << info << "Set configuration space regex to '"
               << configuration_space_regex->front() << "'" << std::endl;
        _inConfigurationSpace_regexp = boost::regex(configuration_space_regex->front(), boost::regex::perl);
    } else
        _inConfigurationSpace_regexp = boost::regex("^CONFIG_[^ ]+$", boost::regex::perl);

    if (_cnf->getVarCount() == 0) {
        // if the model is empty (e.g., if /dev/null was loaded), it cannot possibly be complete
        _cnf->addMetaValue("CONFIGURATION_SPACE_INCOMPLETE", "1");
    }
}

CnfConfigurationModel::~CnfConfigurationModel() {
    delete _model_stream;
    delete _cnf;
}

void CnfConfigurationModel::addFeatureToWhitelist(const std::string feature) {
    const std::string magic("ALWAYS_ON");
    _cnf->addMetaValue(magic, feature);
}

const StringList *CnfConfigurationModel::getWhitelist() const {
    const std::string magic("ALWAYS_ON");
    return _cnf->getMetaValue(magic);
}

void CnfConfigurationModel::addFeatureToBlacklist(const std::string feature) {
    const std::string magic("ALWAYS_OFF");
    _cnf->addMetaValue(magic, feature);
}

const StringList *CnfConfigurationModel::getBlacklist() const {
    const std::string magic("ALWAYS_OFF");
    return _cnf->getMetaValue(magic);
}

std::set<std::string> CnfConfigurationModel::findSetOfInterestingItems(const std::set<std::string> &initialItems) const {
    std::set<std::string> result;
    return result;
}

int CnfConfigurationModel::doIntersect(const std::string exp,
                                    const ConfigurationModel::Checker *c,
                                    std::set<std::string> &missing,
                                    std::string &intersected) const {
    const std::set<std::string> start_items = ConditionalBlock::itemsOfString(exp);
    return doIntersect(start_items, c, missing, intersected);
}


int CnfConfigurationModel::doIntersect(const std::set<std::string> start_items,
                                    const ConfigurationModel::Checker *c,
                                    std::set<std::string> &missing,
                                    std::string &intersected) const {
    StringJoiner sj;
    int valid_items = 0;


    const std::string magic_on("ALWAYS_ON");
    const std::string magic_off("ALWAYS_OFF");
    const StringList *always_on = this->getMetaValue(magic_on);
    const StringList *always_off = this->getMetaValue(magic_off);

    for(std::set<std::string>::const_iterator it = start_items.begin(); it != start_items.end(); it++) {

        if (containsSymbol(*it)) {
            valid_items++;

            if (always_on) {
                StringList::const_iterator cit = std::find(always_on->begin(), always_on->end(), *it);
                if (cit != always_on->end()) {
                    sj.push_back(*it);
                }
            }

            if (always_off) {
                StringList::const_iterator cit = std::find(always_off->begin(), always_off->end(), *it);
                if (cit != always_off->end()) {
                    sj.push_back("!" + *it);
                }
            }

        } else {
            // check if the symbol might be in the model space.
            // if not it can't be missing!
            logger << debug << *it  << std::endl;
            if (!inConfigurationSpace(*it))
                continue;

            // iff we are given a checker for items, skip if it doesn't pass the test
            if (c && ! (*c)(*it)) {
                continue;
            }

            /* free variables are never missing */
            if (it->size() > 1 && !boost::starts_with(*it, "__FREE__"))
                missing.insert(*it);
        }
    }
    sj.push_back("._." + _name + "._.");
    intersected = sj.join("\n&& ");
    logger << debug << "Out of " << start_items.size() << " items "
           << missing.size() << " have been put in the MissingSet" << " using " << _name
           << std::endl;

    return valid_items;
}

bool CnfConfigurationModel::inConfigurationSpace(const std::string &symbol) const {
    if (boost::regex_match(symbol, _inConfigurationSpace_regexp))
        return true;
    return false;
}

bool CnfConfigurationModel::isComplete() const {
    const StringList *configuration_space_complete = _cnf->getMetaValue("CONFIGURATION_SPACE_INCOMPLETE");
    // Reverse logic at this point to ensure Legacy models for kconfig
    // to work
    return !(configuration_space_complete != NULL);
}

bool CnfConfigurationModel::isBoolean(const std::string &item) const {
    return _cnf->getSymbolType(item) == 1;
}

bool CnfConfigurationModel::isTristate(const std::string &item) const {
    return _cnf->getSymbolType(item) == 2;
}

std::string CnfConfigurationModel::getType(const std::string &feature_name) const {
    static const boost::regex item_regexp("^(CONFIG_)?([0-9A-Za-z_]+)(_MODULE)?$");
    boost::match_results<std::string::const_iterator> what;

    if (boost::regex_match(feature_name, what, item_regexp)) {
        std::string item = what[2];
        int type = _cnf->getSymbolType(item);
        static const char *types[] = { "MISSING", "BOOLEAN", "TRISTATE", "INTEGER", "HEX", "STRING", "other"} ;
        return std::string(types[type]);
    }
    return std::string("#ERROR");
}

bool CnfConfigurationModel::containsSymbol(const std::string &symbol) const {
    if (symbol.substr(0,5) == "FILE_") {
        return true;
    }

    if (_cnf->getAssociatedSymbol(symbol) != 0) {
        return true;
    }

    return false;
}
