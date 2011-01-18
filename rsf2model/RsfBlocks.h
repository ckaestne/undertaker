/*
 *   rsf2model - convert dumpconf output to undertaker model format
 *
 * Copyright (C) 2010-2011 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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
#ifndef rsfblock_h__
#define rsfblock_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>


class RsfBlocks : public std::multimap<std::string, std::deque<std::string> > {
public:

    RsfBlocks(std::ifstream &f, std::string relation, std::ostream &log=std::cout);

    std::string *getValue(std::string key);
    std::string *getValue(int n);
    void print_contents(std::ostream &out);

private:
    void read_blocks(std::ifstream &rsf_file, std::string relation);
    std::ostream &log;
    std::string relation;

};

#endif
