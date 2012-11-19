// Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
// Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
// Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef TRACEUTIL_STDIO_H
#define TRACEUTIL_STDIO_H

#include <stdio.h>
#include "constants.h"
#include "hashset.h"

#define TO_STR(x) #x
#define STR(x) TO_STR(x)


/* To determine if an address belongs to a certain module, we use the
   following struct.
 * The members represent the following information:
 * - base:   the modules base address in memory, i.e. where it starts
 * - length: size of the module
 * - name:   the modules name, needed to find the .ko file later
 */

typedef struct module {
    unsigned long long base;
    unsigned long long length;
    char name[MODULENAMELENGTH+3];
} module;

//function declarations
inline void readModules();
inline bool addAddr(unsigned long long addr);
inline bool addModuleAddr(unsigned long long addr);
inline void ignoreFunc(char *name);

#endif
