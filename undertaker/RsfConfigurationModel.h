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


// -*- mode: c++ -*-
#ifndef rsf_configuration_model_h__
#define rsf_configuration_model_h__

#include <string>
#include <map>
#include <deque>
#include <set>
#include <list>
#include <boost/regex.hpp>

#include "ConfigurationModel.h"

std::list<std::string> itemsOfString(const std::string &str);

class RsfConfigurationModel: public ConfigurationModel {
public:

    //! Loads the configuration model from file
    /*!
     * \param name the architecture
     * \param in   istream of the model file as produced by rsf2model.
                   Will be deleted upon object destruction.
     * \param rsf  istream of the rsf file. used for type information of symbol files
                   Will be deleted upon object destruction.
     * \param log  error stream for diagnostics
     */
    RsfConfigurationModel(std::string name, std::istream *in, std::istream *rsf);

    //! destructor
    ~RsfConfigurationModel();

    //! add feature to whitelist ('ALWAYS_ON')
    void addFeatureToWhitelist(const std::string feature);

    //! gets the current feature whitelist ('ALWAYS_ON')
    /*!
     * NB: The referenced list gets invalidated by addFeatureToWhitelist!
     *
     * The referenced object must not be freed, the model class manages it.
     */
    const StringList *getWhitelist() const;

    //! add feature to blacklist ('ALWAYS_OFF')
    /*!
     * NB: This invalidates possibly returned StringList objects
     * referenced by getWhitelist(). Be sure to call getWhitelist()
     * again after using this method.
     */
    void addFeatureToBlacklist(const std::string feature);

    //!< gets the current feature blacklist ('ALWAYS_OFF')
    const StringList *getBlacklist() const;


    int doIntersect(const std::string exp,
                    const ConfigurationModel::Checker *c,
                    std::set<std::string> &missing,
                    std::string &intersected) const;

    int doIntersect(const std::set<std::string> exp,
                    const ConfigurationModel::Checker *c,
                    std::set<std::string> &missing,
                    std::string &intersected) const;

    std::set<std::string> findSetOfInterestingItems(const std::set<std::string> &working) const;
    std::string getName() const { return _name; }

    //! checks if a given item should be in the model space
    bool inConfigurationSpace(const std::string &symbol) const;

    //! checks if we can assume that the configuration space is complete
    bool isComplete() const;

    //@{
    //! checks the type of a given symbol.
    //! @return false if not found
    bool isBoolean(const std::string&) const;
    bool isTristate(const std::string&) const;
    //@}

    //! returns the type of the given symbol
    /*!
     * Normalizes the given item so that passing with and without
     * CONFIG_ prefix works.
     */
    std::string getType(const std::string &feature_name) const;

    RsfReader::iterator find(const RsfReader::key_type &x) const { return _model->find(x); }
    RsfReader::iterator begin() const { return _model->begin(); }
    RsfReader::iterator end()   const { return _model->end(); }

    bool containsSymbol(std::string symbol) { return (this->find(symbol) != this->end()); }

    const StringList *getMetaValue(const std::string &key) const {
        return _model->getMetaValue(key);
    }


private:
    boost::regex _inConfigurationSpace_regexp;
    std::istream *_model_stream;
    std::istream *_rsf_stream;
    RsfReader *_model;
    ItemRsfReader *_rsf;
};

#endif
