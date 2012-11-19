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

#ifndef TRACEUTIL_NATIVE_H
#define TRACEUTIL_NATIVE_H

#include <fcntl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdbool.h>
#include "constants.h"
#include "hashset.h"

/* parser configuration
 * definition of the stages for incremental checking for the different
 * information we want to get out of an ftrace output line.
 */
#define FTRACE_DROP                    -1
#define FTRACE_CHECK_COMMENT            0
#define FTRACE_IGNORE_FRONT             1
#define FTRACE_IGNORE_SPACE             2
#define FTRACE_READ_FUNCTION_NAME       3
#define FTRACE_IGNORE_ZERO              4
#define FTRACE_IGNORE_X                 5
#define FTRACE_READ_OFFSET              6
#define FTRACE_FIND_BASE_ADDR           7
#define FTRACE_READ_BASE_ADDR           8
#define FTRACE_FIND_CALLER_BASE_ADDR    9
#define FTRACE_READ_CALLER_BASE_ADDR   10

/* similar to the parser above
 * here we define the stages to check for the information from /proc/modules
 */
#define MODULES_DROP                   -1
#define MODULES_READ_NAME               0
#define MODULES_READ_LENGTH             1
#define MODULES_FIND_ZERO               2
#define MODULES_IGNORE_X                3
#define MODULES_READ_BASE_ADDR          4

#define STRLEN(s) (sizeof(s)/sizeof(s[0]))
#define error(x)  write(STDERR_FILENO,x,STRLEN(x));

/* To determine if an address belongs to a certain module, we use the following
 * structure. Its members represent the following information:
 * - base:     the modules base address in memory, i.e. where it starts
 * - length:   size of the module
 * - name:     the modules name, needed to later find the corresponding .ko file
 */
typedef struct module {
    uintptr_t base;
    uintptr_t length;
    char name[MODULENAMELENGTH+3];
} module;

//hexmap for fast conversion
int hexCharToDec[256];
char decToHexChar[16];

//function declarations
void readModules();
inline bool addModuleAddr(unsigned long long addr);
inline bool addAddr(unsigned long long addr);
inline void ignoreFunc(char * funcbuf, int funcbufp);

#endif
