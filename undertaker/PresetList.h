/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
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
#ifndef presetlist_h__
#define presetlist_h__

#include <list>
#include <string>
#include <istream>
#include "StringJoiner.h"

/**
 * \brief Manages the black- and whitelist of Kconfig items (like the pure whitelist)
 */
struct PresetList : protected StringJoiner {
    static PresetList *getInstance(); //!< accessor for this singleton
    bool isBlacklisted(const char *item) const { return isListed(getNegatedName(item).c_str()); } //!< checks if the given item is in the blacklist
    bool isWhitelisted(const char *item) const { return isListed(item); } //!< checks if the given item is in the whitelist
    void addToBlacklist(const std::string); //!< adds an item to the blacklist
    void addToWhitelist(const std::string); //!< adds an item to the whitelist
    std::string toSatStr(); //!< returns a string representation

    /**
     * \brief load Kconfig Items from a textfile into the blacklist
     * \param file the filename to load items from
     * \return the number of items that where loaded
     */
    int loadBlacklist(std::istream &file){ return loadList(file,false); }

    /**
     * \brief load Kconfig Items from a textfile into the whitelist
     * \param file the filename to load items from
     * \return the number of items that where loaded
     */
    int loadWhitelist(std::istream &file){ return loadList(file,true); }

private:
    std::string getNegatedName(const char *) const;
    int loadList(std::istream &list, bool white);
    bool isListed(const char*) const;
    bool isValid(const char*) const;
    PresetList() : StringJoiner() {} //!< private c'tor
};

#endif
