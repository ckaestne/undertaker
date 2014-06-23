/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


// -*- mode: c++ -*-
#ifndef configuration_model_h__
#define configuration_model_h__

#include "RsfReader.h" // for 'StringList'

#include <string>
#include <set>
#include <boost/regex.hpp>


class ConfigurationModel {
public:
    struct Checker {
        //! checks if the item is a candidate for addition in the 'missing' set
        virtual bool operator()(const std::string &item) const = 0;
    };

    //! destructor
    virtual ~ConfigurationModel() {};

    //! add feature to whitelist ('ALWAYS_ON')
    virtual void addFeatureToWhitelist(const std::string feature) = 0;

    //! gets the current feature whitelist ('ALWAYS_ON')
    /*!
     * NB: The referenced list gets invalidated by addFeatureToWhitelist!
     *
     * The referenced object must not be freed, the model class manages it.
     */
    virtual const StringList *getWhitelist() const = 0;

    //! add feature to blacklist ('ALWAYS_OFF')
    /*!
     * NB: This invalidates possibly returned StringList objects
     * referenced by getWhitelist(). Be sure to call getWhitelist()
     * again after using this method.
     */
    virtual void addFeatureToBlacklist(const std::string feature) = 0;

    //!< gets the current feature blacklist ('ALWAYS_OFF')
    virtual const StringList *getBlacklist() const = 0;


    virtual int doIntersect(const std::string exp,
                    const ConfigurationModel::Checker *c,
                    std::set<std::string> &missing,
                    std::string &intersected) const = 0;

    virtual int doIntersect(const std::set<std::string> exp,
                    const ConfigurationModel::Checker *c,
                    std::set<std::string> &missing,
                    std::string &intersected) const = 0;

    virtual std::set<std::string> findSetOfInterestingItems(const std::set<std::string> &working) const = 0;
    static std::string getMissingItemsConstraints(const std::set<std::string> &missing);

    std::string getName() const { return _name; }

    //! returns the version identifier for the current model
    virtual const char *getModelVersionIdentifier() const = 0;

    //! checks if a given item should be in the model space
    virtual bool inConfigurationSpace(const std::string &symbol) const = 0;

    //! checks if we can assume that the configuration space is complete
    virtual bool isComplete() const = 0;

    //@{
    //! checks the type of a given symbol.
    //! @return false if not found
    virtual bool isBoolean(const std::string&) const = 0;
    virtual bool isTristate(const std::string&) const = 0;
    //@}

    //! returns the type of the given symbol
    /*!
     * Normalizes the given item so that passing with and without
     * CONFIG_ prefix works.
     */
    virtual std::string getType(const std::string &feature_name) const = 0;

    virtual bool containsSymbol(const std::string &symbol) const = 0;

    virtual const StringList *getMetaValue(const std::string &key) const = 0;


protected:
    std::string _name;
};

#endif
