// -*- mode: C++ -*-
/*
 *   libziz - parse c preprocessor files
 * Copyright (C) 2010 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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


#ifndef ZIZLER_H_
#define ZIZLER_H_

#include "Ziz.h"

#include <string>

typedef enum {
    Short,
    Medium,
    Long,
    Convert
} Mode;

std::ostream & operator+ (std::ostream &stream, Ziz::File             const &);
std::ostream & operator+ (std::ostream &stream, Ziz::Block            const &);
std::ostream & operator+ (std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator* (std::ostream &stream, Ziz::File             const &);
std::ostream & operator* (std::ostream &stream, Ziz::Block            const &);
std::ostream & operator* (std::ostream &stream, Ziz::ConditionalBlock const &);
std::ostream & operator* (std::ostream &stream, Ziz::CodeBlock        const &);

std::ostream & operator<<(std::ostream &stream, Ziz::File             const &);
std::ostream & operator<<(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator<<(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator<<(std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator>>(std::ostream &stream, Ziz::File             const &);
std::ostream & operator>>(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator>>(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator>>(std::ostream &stream, Ziz::ConditionalBlock const &);

void chomp(std::string &s);
std::string Indent(int depth);

bool ziztest(std::string file, Mode mode);

#endif /* ZIZLER_H_ */
