/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#ifdef DEBUG
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif

#include "RsfConfigurationModel.h"
#include "Tools.h"
#include "StringJoiner.h"
#include "RsfReader.h"
#include "Logging.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <fstream>
#include <stack>


RsfConfigurationModel::RsfConfigurationModel(const std::string &filename) {
    const StringList *configuration_space_regex;
    boost::filesystem::path filepath(filename);
    _name = filepath.stem().string();

    _model_stream = new std::ifstream(filename);

    if (filename != "/dev/null" && _model_stream->good()) {
        bool have_rsf = false;

        if (filepath.extension() == ".model") {
            filepath.replace_extension(".rsf");
            _rsf_stream = new std::ifstream(filepath.string());
            have_rsf = true;
        } else {
            _rsf_stream = new std::ifstream("/dev/null");
        }
        if (!have_rsf || !_rsf_stream->good()) {
            Logging::warn("could not open file for reading: ", filename);
            Logging::warn("checking the type of symbols will fail");
        }
    } else {
        delete _model_stream;
        _model_stream = new std::ifstream("/dev/null");
        _rsf_stream = new std::ifstream("/dev/null");
    }
    _model = new RsfReader(*_model_stream, "UNDERTAKER_SET");
    _rsf   = new ItemRsfReader(*_rsf_stream);

    configuration_space_regex = _model->getMetaValue("CONFIGURATION_SPACE_REGEX");

    if (configuration_space_regex != nullptr && configuration_space_regex->size() > 0) {
        Logging::info("Set configuration space regex to '", configuration_space_regex->front(),
                      "'");
        _inConfigurationSpace_regexp = boost::regex(configuration_space_regex->front(), boost::regex::perl);
    } else {
        _inConfigurationSpace_regexp = boost::regex("^CONFIG_[^ ]+$", boost::regex::perl);
    }
    if (_model->size() == 0) {
        // if the model is empty (e.g., if /dev/null was loaded), it cannot possibly be complete
        _model->addMetaValue("CONFIGURATION_SPACE_INCOMPLETE", "1");
    }
}

RsfConfigurationModel::~RsfConfigurationModel() {
    delete _model;
    delete _rsf;
    delete _rsf_stream;
    delete _model_stream;
}

void RsfConfigurationModel::addFeatureToWhitelist(const std::string feature) {
    const std::string magic("ALWAYS_ON");
    _model->addMetaValue(magic, feature);
}

const StringList *RsfConfigurationModel::getWhitelist() const {
    const std::string magic("ALWAYS_ON");
    return _model->getMetaValue(magic);
}

void RsfConfigurationModel::addFeatureToBlacklist(const std::string feature) {
    const std::string magic("ALWAYS_OFF");
    _model->addMetaValue(magic, feature);
}

const StringList *RsfConfigurationModel::getBlacklist() const {
    const std::string magic("ALWAYS_OFF");
    return _model->getMetaValue(magic);
}

std::set<std::string> RsfConfigurationModel::findSetOfInterestingItems(const std::set<std::string> &initialItems) const {
    std::set<std::string> result;
    std::stack<std::string> workingStack;
    /* Initialize the working stack with the given elements */
    for (const std::string &str : initialItems) {
        workingStack.push(str);
        result.insert(str);
    }
    while (!workingStack.empty()) {
        const std::string *item = _model->getValue(workingStack.top());
        workingStack.pop();
        if (item != nullptr && item->compare("") != 0) {
            for (const std::string &str : undertaker::itemsOfString(*item)) {
                /* Item already seen? continue */
                if (result.count(str) == 0) {
                    workingStack.push(str);
                    result.insert(str);
                }
            }
        }
    }
    return result;
}

int RsfConfigurationModel::doIntersect(const std::string exp,
                                    const ConfigurationModel::Checker *c,
                                    std::set<std::string> &missing,
                                    std::string &intersected) const {
    const std::set<std::string> start_items = undertaker::itemsOfString(exp);
    return doIntersect(start_items, c, missing, intersected);
}


int RsfConfigurationModel::doIntersect(const std::set<std::string> start_items,
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

    // ALWAYS_ON and ALWAYS_OFF items and their transitive dependencies
    // always need to appear in the slice.
    if (always_on) {
        for (const std::string &str : *always_on)
            interesting.insert(str);
    }
    if (always_off) {
        for (const std::string &str : *always_off)
            interesting.insert(str);
    }

    for (const std::string &str : interesting) {
        const std::string *item = _model->getValue(str);

//        Logging::debug("interesting item: ", str);
        if (item != nullptr) {
            valid_items++;
            if (item->compare("") != 0) {
                sj.push_back("(" + str + " -> (" + *item + "))");
            }
            if (always_on) {
                const auto &cit = std::find(always_on->begin(), always_on->end(), str);
                if (cit != always_on->end()) // str is found
                    sj.push_back(str);
            }
            if (always_off) {
                const auto &cit = std::find(always_off->begin(), always_off->end(), str);
                if (cit != always_off->end()) // str is found
                    sj.push_back("!" + str);
            }
        } else {
            // check if the symbol might be in the model space. if not it can't be missing!
            if (!inConfigurationSpace(str))
                continue;

            // if we are given a checker for items, skip if it doesn't pass the test
            if (c && ! (*c)(str))
                continue;

            /* free variables are never missing */
            if (str.size() > 1 && !boost::starts_with(str, "__FREE__"))
                missing.insert(str);
        }
    }
    intersected = sj.join("\n&& ");
    Logging::debug("Out of ", start_items.size(), " items ", missing.size(),
                   " have been put in the MissingSet");
    return valid_items;
}

bool RsfConfigurationModel::inConfigurationSpace(const std::string &symbol) const {
    if (boost::regex_match(symbol, _inConfigurationSpace_regexp)) {
        return true;
    }
    return false;
}

bool RsfConfigurationModel::isComplete() const {
    const StringList *configuration_space_complete = _model->getMetaValue("CONFIGURATION_SPACE_INCOMPLETE");
    // Reverse logic at this point to ensure Legacy models for kconfig to work
    return !(configuration_space_complete != nullptr);
}

bool RsfConfigurationModel::isBoolean(const std::string &item) const {
    const std::string *value = _rsf->getValue(item);

    if (value && 0 == value->compare("boolean")) {
        return true;
    }
    return false;
}

bool RsfConfigurationModel::isTristate(const std::string &item) const {
    const std::string *value = _rsf->getValue(item);

    if (value && 0 == value->compare("tristate")) {
        return true;
    }
    return false;
}

std::string RsfConfigurationModel::getType(const std::string &feature_name) const {
    static const boost::regex item_regexp("^CONFIG_([0-9A-Za-z_]+)(_MODULE)?$");
    boost::smatch what;

    if (boost::regex_match(feature_name, what, item_regexp)) {
        std::string item = what[1];
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
