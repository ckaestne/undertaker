/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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
#ifndef rsfreader_h__
#define rsfreader_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>


typedef std::deque<std::string> StringList;
typedef std::map<std::string, StringList> RsfMap;

/**
 * \brief Reads RSF files
 */
class RsfReader : public RsfMap {
public:

    RsfReader(std::ifstream &f, std::ostream &log=std::cout,
              const std::string metaflag = "");

    const std::string *getValue(const std::string &key) const;
    const StringList *getMetaValue(const std::string &key) const;

    void print_contents(std::ostream &out);

private:
    std::map<std::string, StringList> meta_information;
    StringList parse(const std::string& line);
    void read_rsf(std::ifstream &rsf_file);
    std::ostream &log;
    std::string metaflag;
};

#endif
