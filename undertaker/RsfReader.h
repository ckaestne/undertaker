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
#ifndef rsfreader_h__
#define rsfreader_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>

typedef std::deque<std::string> StringList;


/**
 * \brief Reads RSF files
 */
class RsfReader : public std::map<std::string, StringList> {
public:

    RsfReader(std::istream &f, const std::string metaflag = "");
    virtual ~RsfReader() = default;

    const std::string *getValue(const std::string &key) const;
    const StringList *getMetaValue(const std::string &key) const;

    //! adds value to key in meta_information
    void addMetaValue(const std::string &key, const std::string &value);
    void print_contents(std::ostream &out);

protected:
    RsfReader() = default;
    std::map<std::string, StringList> meta_information;
    StringList parse(const std::string& line);
    virtual size_t read_rsf(std::istream &rsf_file);
    std::string metaflag;
};

/**
 * \brief Special RSF reader that only reads 'Item' lines
 *
 * An RSF file as produced by dumpconf will in general contain a line
 * with the key 'Item' for each Kconfig option, i.e., we will expect key
 * collisions. Since RsfReader is based on a std::map, the key needs to
 * be unique.
 *
 * This RsfReader 'skips' the first 'Item' line. The key of this Map is
 * the item name, the value is the type of the item.
 */
class ItemRsfReader : public RsfReader {
public:
    ItemRsfReader(std::istream &f);
    virtual size_t read_rsf(std::istream &rsf_file) final override;
};

#endif
