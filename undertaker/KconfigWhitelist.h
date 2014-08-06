/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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
#ifndef kconfigwhitelist_h__
#define kconfigwhitelist_h__

#include <vector>
#include <string>

/**
 * \brief Manages Lists of Kconfig Items
 *
 * This class manages three lists: a whitelist, a blacklist and an ignorelist.
 * Each of these lists can be accessed individually.
 *
 * This class follows the singleton pattern, but manages three
 * instances, one for each list.
 */
class KconfigWhitelist : public std::vector<std::string> {
    KconfigWhitelist() = default;      //!< private c'tor
public:
    static KconfigWhitelist &getIgnorelist();  //!< ignorelist
    static KconfigWhitelist &getWhitelist();   //!< whitelist
    static KconfigWhitelist &getBlacklist();   //!< blacklist
    //!< checks if the given item is in the whitelist
    bool isWhitelisted(const std::string &s) const;
    /**
     * \brief load Kconfig Items from a textfile into the whitelist
     * \param file the filename to load items from
     * \return the number of items that where loaded
     */
    int loadWhitelist(const char *file);
};

#endif
