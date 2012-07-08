/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
#include "ConfigurationModel.h"
#include "KconfigWhitelist.h"
#include "StringJoiner.h"
#include "RsfReader.h"
#include "Logging.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/regex.hpp>
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <sstream>
#include <fstream>
#include <list>
#include <stack>


ConfigurationModel::ConfigurationModel(std::string name, std::istream *in, std::istream *rsf)
    :  _name(name), _model_stream(in), _rsf_stream(rsf) {
    const StringList *configuration_space_regex;

    _model = new RsfReader(*_model_stream, "UNDERTAKER_SET");
    _rsf   = new ItemRsfReader(*_rsf_stream);

    configuration_space_regex = _model->getMetaValue("CONFIGURATION_SPACE_REGEX");

    if (configuration_space_regex != NULL && configuration_space_regex->size() > 0) {
        logger << info << "Set configuration space regex to '"
               << configuration_space_regex->front() << "'" << std::endl;
        _inConfigurationSpace_regexp = boost::regex(configuration_space_regex->front(), boost::regex::perl);
    } else
        _inConfigurationSpace_regexp = boost::regex("^CONFIG_[^ ]+$", boost::regex::perl);

    if (_model->size() == 0) {
        // if the model is empty (e.g., if /dev/null was loaded), it cannot possibly be complete
        _model->addMetaValue("CONFIGURATION_SPACE_INCOMPLETE", "1");
    }
}

ConfigurationModel::~ConfigurationModel() {
    delete _model;
    delete _rsf;
    delete _rsf_stream;
    delete _model_stream;
}

void ConfigurationModel::addFeatureToWhitelist(const std::string feature) {
    const std::string magic("ALWAYS_ON");
    _model->addMetaValue(magic, feature);
}

const StringList *ConfigurationModel::getWhitelist() const {
    const std::string magic("ALWAYS_ON");
    return _model->getMetaValue(magic);
}

void ConfigurationModel::addFeatureToBlacklist(const std::string feature) {
    const std::string magic("ALWAYS_OFF");
    _model->addMetaValue(magic, feature);
}

const StringList *ConfigurationModel::getBlacklist() const {
    const std::string magic("ALWAYS_OFF");
    return _model->getMetaValue(magic);
}

std::set<std::string> ConfigurationModel::findSetOfInterestingItems(const std::set<std::string> &initialItems) const {
    std::set<std::string> item_set, result;
    std::stack<std::string> workingStack;
    std::string tmp;

    /* Initialize the working stack with the given elements */
    for(std::set<std::string>::iterator sit = initialItems.begin(); sit != initialItems.end(); sit++) {
        workingStack.push(*sit);
        result.insert(*sit);
    }

    while (!workingStack.empty()) {
        const std::string *item = _model->getValue(workingStack.top());
        workingStack.pop();
        if (item != NULL && item->compare("") != 0) {
            item_set = ConditionalBlock::itemsOfString(*item);
            for(std::set<std::string>::const_iterator sit = item_set.begin(); sit != item_set.end(); sit++) {
                /* Item already seen? continue */
                if (result.count(*sit) == 0) {
                    workingStack.push(*sit);
                    result.insert(*sit);
                }
            }
        }
    }

    return result;
}

std::string ConfigurationModel::getMissingItemsConstraints(const std::set<std::string> &missing) {
    StringJoiner sj;

    for (std::set<std::string>::const_iterator it = missing.begin(); it != missing.end(); it++)
        sj.push_back(*it);

    std::stringstream ss;
    if (sj.size() > 0)
        ss << "( ! ( " <<  sj.join(" || ") << " ) )";
    return ss.str();
}

int ConfigurationModel::doIntersect(const std::string exp,
                                    const ConfigurationModel::Checker *c,
                                    std::set<std::string> &missing,
                                    std::string &intersected) const {
    const std::set<std::string> start_items = ConditionalBlock::itemsOfString(exp);
    return doIntersect(start_items, c, missing, intersected);
}


int ConfigurationModel::doIntersect(const std::set<std::string> start_items,
                                    const ConfigurationModel::Checker *c,
                                    std::set<std::string> &missing,
                                    std::string &intersected) const {
    int valid_items = 0;
    StringJoiner sj;

    std::set<std::string> interesting = findSetOfInterestingItems(start_items);

    const std::string magic_on("ALWAYS_ON");
    const std::string magic_off("ALWAYS_OFF");
    const StringList *always_on = this->getMetaValue(magic_on);
    const StringList *always_off = this->getMetaValue(magic_off);

    for(std::set<std::string>::const_iterator it = interesting.begin(); it != interesting.end(); it++) {
        const std::string *item = _model->getValue(*it);

        // logger << debug << "interesting item: " << *it << std::endl;

        if (item != NULL) {
            valid_items++;
            if (item->compare("") != 0)
                sj.push_back("(" + *it + " -> (" + *item + "))");

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
    intersected = sj.join("\n&& ");

    logger << debug << "Out of " << start_items.size() << " items "
           << missing.size() << " have been put in the MissingSet"
           << std::endl;

    return valid_items;
}

bool ConfigurationModel::inConfigurationSpace(const std::string &symbol) const {
    if (boost::regex_match(symbol, _inConfigurationSpace_regexp))
        return true;
    return false;
}

bool ConfigurationModel::isComplete() const {
    const StringList *configuration_space_complete = _model->getMetaValue("CONFIGURATION_SPACE_INCOMPLETE");
    // Reverse logic at this point to ensure Legacy models for kconfig
    // to work
    return !(configuration_space_complete != NULL);
}

bool ConfigurationModel::isBoolean(const std::string &item) const {
    const std::string *value = _rsf->getValue(item);

    if (value && 0 == value->compare("boolean"))
        return true;

    return false;
}

bool ConfigurationModel::isTristate(const std::string &item) const {
    const std::string *value = _rsf->getValue(item);

    if (value && 0 == value->compare("tristate"))
        return true;

    return false;
}

std::string ConfigurationModel::getType(const std::string &feature_name) const {
    static const boost::regex item_regexp("^(CONFIG_)?([0-9A-Za-z_]+)(_MODULE)?$");
    boost::match_results<std::string::const_iterator> what;

    if (boost::regex_match(feature_name, what, item_regexp)) {
        std::string item = what[2];
        const std::string *value = _rsf->getValue(item);
        if (value) {
            std::string type = *value;
            std::transform(type.begin(), type.end(), type.begin(), ::toupper);
            return type;
        } else {
            return std::string("MISSING");
        }
    }
    return std::string("#ERROR");
}
