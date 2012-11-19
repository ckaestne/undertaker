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

#include "traceutil-native.h"

// information about loaded kernel modules
module loadedModule[MODULESIZE];
int loadedModules = 0;
uintptr_t moduleBaseAddr = 0;

// path information for required files
const char * modulePath;
const char * tracePath;
const char * ignorePath;
const char * outputPath;

// file descriptors of input and output
int outputFile;
int ignoreFile;

int ignorePathReopenUpperBound = IGNOREFILEREOPENUPPERBOUND;
const int ignorePathReopenLimit = IGNOREFILEREOPENLOWERBOUND;
int ignorePathReopenCounter = 0;

// universal counter variable
int j;

// Outputbuffer
char outputBuffer[INPUTBUFFERSIZE+ADDRLENGTH+MODULENAMELENGTH+3];
int outputSize=0;

// Modulename Buffer
char moduleNameBuffer[INPUTBUFFERSIZE+3];
int moduleNameSize = 0;

// Buffer for converting long to char[]
char numberBuffer[ADDRLENGTH+3];
int numberSize=0;

/* This function reloads Linux's module table. For every module found in the
 * table, we store its name, the base address and its size. By doing so, we can
 * later test whether a given address is inside a certain module's address range
 */

void readModules() {
    int size = 0;
    int parsingMode = 0;
    // Open module file
    int moduleFile = open(modulePath,O_RDONLY);
    if (moduleFile  == -1) {
        error("Error opening modules");
        return;
    }
    loadedModules = 0;
    while ((size = read(moduleFile, moduleNameBuffer, INPUTBUFFERSIZE)) > 0) {
        // go for every char
        for (int position=0; position<size; position++) {
            switch (parsingMode) {
            // Read the modules name
            case MODULES_READ_NAME:
                if (moduleNameBuffer[position] == '\0'
                    || moduleNameBuffer[position] == '\n') {
                    moduleNameSize = 0;
                }
                else if (moduleNameBuffer[position] == ' '
                         || moduleNameSize >= MODULENAMELENGTH ) {
                    // go to next space
                    while (position<size && moduleNameBuffer[position] != ' ') {
                        position++;
                    }
                    // terminate string
                    loadedModule[loadedModules].name[moduleNameSize] = '\0' ;
                    loadedModule[loadedModules].base = 0 ;
                    loadedModule[loadedModules].length = 0 ;
                    moduleNameSize = 0;
                    parsingMode = MODULES_READ_LENGTH;
                }
                else {
                    loadedModule[loadedModules].name[moduleNameSize++] =
                            moduleNameBuffer[position] ;
                }
                continue;
                // Get length of the module (this is given in decimal)
            case MODULES_READ_LENGTH:
                if (moduleNameBuffer[position]>=48
                    && moduleNameBuffer[position]<=57){
                    loadedModule[loadedModules].length =
                            loadedModule[loadedModules].length * 10
                            + hexCharToDec[(int) moduleNameBuffer[position]];
                } else {
                    parsingMode = MODULES_FIND_ZERO;
                }
                continue;
                // Try to find the begin of the address
            case MODULES_FIND_ZERO:
                if (moduleNameBuffer[position] == '0') {
                    parsingMode = MODULES_IGNORE_X;
                }
                continue;
                // Switch for a possible given 0x prefix
            case MODULES_IGNORE_X:
                if (moduleNameBuffer[position] == 'x') {
                    parsingMode = MODULES_READ_BASE_ADDR;
                } else {
                    parsingMode = MODULES_FIND_ZERO;
                }
                continue;
                // get the base address
            case MODULES_READ_BASE_ADDR:
                // This is the case if parsing the addresses is finished
                if (hexCharToDec[(int) moduleNameBuffer[position]] == -1) {
                    // Set memory base address
                    // this is the lowest memory address of a module
                    if (moduleBaseAddr == 0
                        || moduleBaseAddr > loadedModule[loadedModules].base) {
                        moduleBaseAddr = loadedModule[loadedModules].base;
                    }
                    // ready for next module
                    loadedModules++;
                    if (moduleNameBuffer[position] == '\n') {
                        parsingMode = MODULES_READ_NAME;
                    } else {
                        parsingMode = MODULES_DROP;
                    }
                }
                // Read next hex digit and add it to the address
                else {
                    loadedModule[loadedModules].base =
                            loadedModule[loadedModules].base * 16
                            + hexCharToDec[(int) moduleNameBuffer[position]];
                }
                continue;
                // Wait for newline
            default:
                if (moduleNameBuffer[position] == '\n') {
                    parsingMode = MODULES_READ_NAME;
                }
            }
        }
    }
    if (size == -1) {
        error("Error reading modules");
    } else if (close(moduleFile) == -1) {
        error("Error closing modules");
    }
    return;
}

/* This function adds an address from an LKM to the output. Therefore we iterate
 * the table of modules, and check if the address is between the lower and the
 * upper bound of any module. If we find the module, we return *true* to signal
 * success. If we don't find any module matching, we return *false*; this will
 * tell the caller to call readModules() and therefore rescan the module table
 * currently present.
 */

inline bool addModuleAddr(unsigned long long addr) {
    for (j=0; j<loadedModules; j++) {
        unsigned long long lowAddr = loadedModule[j].base;
        unsigned long long highAddr = lowAddr + loadedModule[j].length;
        if (addr>=lowAddr && addr<=highAddr) {
            moduleNameSize = j;

            addr -= loadedModule[moduleNameSize].base;
            // output address + func name
            outputSize = 0;
            // converting long to print
            // Process reversed long str representation
            numberSize = 0;
            if (addr == 0) {
                numberBuffer[numberSize++] = '0';
            } else {
                while (addr > 0 && numberSize < ADDRLENGTH) {
                    numberBuffer[numberSize++] = decToHexChar[addr%16];
                    addr /= 16;
                }
                // copy reversed
                for (j=numberSize-1; j>=0; j--) {
                    outputBuffer[outputSize++]=numberBuffer[j];
                }
                // Print module name
                outputBuffer[outputSize++]=' ';
                for (j=0; loadedModule[moduleNameSize].name[j]!='\0'; j++) {
                    outputBuffer[outputSize++] =
                            loadedModule[moduleNameSize].name[j];
                }
            }
            // terminate string
            outputBuffer[outputSize++]='\n';
            outputBuffer[outputSize]='\0';
            // output
            return write(outputFile,outputBuffer,outputSize)>0 ? true : false;
        }
    }
    return false;
}

/* This function adds a found address to the output. It also adds the address to
 * the internal hash map, so that we can filter out already found addresses.  If
 * the given address was located inside an LKM or we didn't scan the module
 * table yet, a call to addModuleAddr(addr) is triggered.
 */

inline bool addAddr(unsigned long long addr) {
    if (hashadd(addr)) {
        // Check if module
        if (modulePath != NULL && addr >= moduleBaseAddr) {
            // ... if not, rescan /proc/modules...
            if (addModuleAddr(addr)) {
                return true;
            } else {
                readModules(j);
                if (addModuleAddr(addr)) {
                    return true;
                }
            }
        }
        // output address + func name
        outputSize = 0;
        // converting long to print
        // Process reversed long str representation
        numberSize = 0;
        if (addr == 0) {
            numberBuffer[numberSize++] = '0';
        } else {
            while (addr > 0 && numberSize < ADDRLENGTH) {
                numberBuffer[numberSize++] = decToHexChar[addr%16];
                addr /= 16;
            }
        }
        // copy reversed
        for (j=numberSize-1; j>=0; j--) {
            outputBuffer[outputSize++]=numberBuffer[j];
        }
        // terminate string
        outputBuffer[outputSize++]='\n';
        outputBuffer[outputSize]='\0';

        // output
        return write(outputFile,outputBuffer,outputSize)>0 ? true : false;
    }
    return false;
}

/* This function adds a function to ftraces ignore list and flushes it. We found
 * that calling flush() on a file in the debugfs had no effect, so we close and
 * reopen the file at given intervals.  As there is a lot written to the file in
 * the beginning, we start off with flushing after 1024 writes, and then
 * progressively divide this number by half, flushing more often as less
 * functions are being encountered.
 */

inline void ignoreFunc(char * callerBuffer, int callerSize) {
    // Flushing does not work on debugfs, so we close and reopen the ignore file
    if (ignoreFile == -1
        || ++ignorePathReopenCounter >= ignorePathReopenUpperBound) {
        ignorePathReopenUpperBound /= 2;
        if (ignorePathReopenUpperBound < ignorePathReopenLimit) {
            ignorePathReopenUpperBound = ignorePathReopenLimit;
        }
        ignorePathReopenCounter = 0;

        if (close(ignoreFile) == -1) {
            error("Error closing ignore");
            return;
        }
        else if ((ignoreFile = open(ignorePath, O_WRONLY | O_CREAT | O_APPEND,
                 S_IRUSR | S_IWUSR | S_IRGRP)) == -1) {
            error("Error opening ignore");
            return;
        }
    }
    write(ignoreFile,callerBuffer,callerSize);
}

/* This is where all the magic happens.  The tool reads the output of ftrace,
 * and converts it into a simpler format. Additionally, it also controls the
 * output of ftrace, dynamically feeding all encountered functions to ftraces
 * own blacklist, so that ftrace will not show them in the log again. We also
 * keep the information from /proc/modules, in order to be able to correctly
 * calculate the offset of an address inside a loaded LKM.
 *
 * This is probably best explained in an example:
 *
 * For a line from the vmlinux binary, the output of ftrace itself looks like
 * the following line:
 *
 * <idle>-0     [001] dN.. 368556.793254: place_entity+0x10/0x90 <ffffffff8108bc70> <-enqueue_entity+0xfd/0x460 <ffffffff8108d57d>
 *
 * From this, we derive:
 * a) the function place_entity is located at 0xffffffff8108bc70-0x10 =
 *    0xffffffff8108bc60 (0x10 is the offset of a function call (mcount) that
 *    is introduced by ftrace. Read the ftrace documentation for further info).
 * b) the function place_entity was called from enqueue_entity, at an offset of
 *    0xfd; this corresponds to the absolute address 0xffffffff8108d57d
 *
 * Both addresses are now added to a hashmap; if they are encountered again,
 * the hashmap is checked if the address to add was found before, and if so, it
 * is not written to the output again.  We now also add the function
 * place_entity to ftraces own blacklist, preventing it showing up in the log
 * again, therefore also greatly reducing the input size for this tool.  The
 * output in our output file produced by the above line will be:
 *
 * ffffffff8108bc60
 * ffffffff8108d57d
 *
 * On x86, LKMs are loaded into the address space at higher addresses than where
 * vmlinux itself is lying.  When first started, our tool will read the contents
 * of /proc/modules, one of which will look like this:
 *
 * async_xor 12879 2 raid456,async_pq, Live 0xffffffffa0098000
 * (name)    (size)                         (LKM base address)
 *
 * The important information we read for every entry is the following:
 * a) the module's name
 * b) the module's base address; this is the last part of the line above
 * c) the module's size: this is the second number in the line.
 * We store all this information in the loadedModule array.
 * Additionally, we store the lowest module base address encountered, which we
 * use as an indicator if the address read from the ftrace output is inside a
 * module or not (being bigger than the lowest module base address therefore
 * means it is from a module). On the x86 systems we tested, the lowest LKM
 * base address always was at 0xffffffffa0000000, so let's consider this being
 * the base address for the following example.
 *
 * If we encounter an address we think of as being part of an LKM, it will look
 * like this in the ftrace output:
 *
 * sshd-11404 [001] ..s. 368580.183184: e1000_maybe_stop_tx+0x14/0x110 [e1000e] <ffffffffa0098564> <-e1000_xmit_frame+0xac9/0xdf0 [e1000e] <ffffffffa0099129>
 *
 * We see that (address-offset) = 0xffffffffa0098550 is bigger than the proposed
 * lowest LKM base address 0xffffffffa0000000, therefore we consider it being
 * part of an LKM. We now iterate through the current state of our view of
 * /proc/modules and check at every entry, if loadedModule[i].base =<
 * 0xffffffffa0098550 <= loadedModule[i].base+length.
 * If we find an entry sufficient to this, we found the module this address
 * belongs to, calculate the offset of the given address inside the module by
 * calculating (0xffffffffa0098550 - lowAddr) and output this result together
 * with the module's name.
 * If we do NOT find an entry for an address, we rescan /proc/modules (maybe the
 * address comes from within a newly loaded module), and try the above again.
 * If this still fails, we only output the address, as this can be caused by a
 * different memory layout.
 * Considering we find the module above at address 0xffffffffa0098000 and its
 * name is e1000e, the output for the line above would be:
 *
 * 564 e1000e
 * 1129 e1000e
 *
 * In order to accomplish all this, we need the following parameters to this
 * tool:

 * - argv[1]: the trace_pipe file, which is the live output of ftrace.
 * - argv[2]: the output file where the parsed information will go in the format
 *            described above
 * - argv[3]: ftrace's own ignore list.
 * - argv[4]: the location of the kernels module list. Usually this will be
 *            /proc/modules (this is also set by the undertaker-tracecontrol
 *            script). We read its contents directly after start, and every
 *            time an address is encountered that is not in any modules range.
 *
 *  We wrote this as a custom parser because we really depend on speed here.
 *  Ftrace will produce massive amounts of data (10 seconds will generate about
 *  600MB!), so we need to be fast to keep up.
 */

int main(int argc, const char *argv[]) {
    /* Initialization */
    // Inputbuffer
    char inputBuffer[INPUTBUFFERSIZE+3];
    // Buffer for function name
    char callerBuffer[FUNCNAMELENGTH+3];
    int callerSize=0;
    // current step in the process of parsing this line
    int parsingMode=0;
    int len,position = 0;
    uintptr_t offset=0;
    uintptr_t address=0;

    // Map for conversion of hexadecimal strings into numbers (and reverse)
    for (position = 0; position<256; position++) {
        hexCharToDec[position] = -1;
    }
    for (position = 0; position<10; position++) {
        hexCharToDec[48+position] = position;
        decToHexChar[position]=(char) (48+position);
        if (position<6) {
            hexCharToDec[65+position] = 10+position;
            hexCharToDec[97+position] = 10+position;
            decToHexChar[10+position] = (char) (97+position);
        }
    }

    /* parse commandline arguments */
    if(argc < 4 ) {
        error("Usage: traceutil <trace> <output> <ignore> [<module>]");
        return 1;
    }
    // Inputfile
    tracePath = argv[1];
    int inputFile = open(tracePath,O_RDONLY);
    if (inputFile == -1) {
        error("Error opening input");
        return 1;
    }

    // Outputfile
    outputPath = argv[2];
    outputFile = open(outputPath, O_WRONLY | O_CREAT | O_NONBLOCK,
                      S_IRUSR | S_IWUSR | S_IRGRP) ;
    if (outputFile == -1) {
        error("Error opening output");
        return 1;
    }

    // ignorePath
    ignorePath = argv[3];
    ignoreFile = open(ignorePath, O_WRONLY | O_CREAT,
                      S_IRUSR | S_IWUSR | S_IRGRP);
    if (ignoreFile == -1) {
        error("Error opening ignore");
        return 1;
    }

    // optional module support
    if (argc < 5) {
        modulePath = NULL;
    }
    else {
        modulePath=argv[4];
        readModules();
    }

    /* Reading trace output */
    while ((len = read(inputFile, inputBuffer, INPUTBUFFERSIZE)) > 0) {
        // go for every char
        for (position=0; position<len; position++) {
            switch (parsingMode) {

            // drop the line if it starts with '#'
            case FTRACE_CHECK_COMMENT:
                if (inputBuffer[position] == '#') {
                    parsingMode = FTRACE_DROP;
                } else {
                    parsingMode = FTRACE_IGNORE_FRONT;
                }
                break;

            // ignore the front until the end of the timestamp
            case FTRACE_IGNORE_FRONT:
                if (inputBuffer[position] == ':') {
                    parsingMode++;
                }
                break;

            /* ": " indicates the beginning of the information we're interested
             * in. There could be a single ':' in the process name, this will be
             * ignored.
             */
            case FTRACE_IGNORE_SPACE:
                if (inputBuffer[position] == ' ') {
                    parsingMode = FTRACE_READ_FUNCTION_NAME;
                } else {
                    parsingMode = FTRACE_IGNORE_FRONT;
                }
                break;

            // read the function name until '+' (i.e. until the offset)
            case FTRACE_READ_FUNCTION_NAME:
                if (inputBuffer[position] == '+') {
                    callerBuffer[callerSize++] = '\n';
                    callerBuffer[callerSize] = '\0';
                    parsingMode=FTRACE_IGNORE_ZERO;
                } else if (callerSize >= FUNCNAMELENGTH) {
                    // ignore name
                    callerSize = 0;
                    parsingMode=FTRACE_IGNORE_ZERO;
                } else {
                    callerBuffer[callerSize++] = inputBuffer[position];
                }
                break;

            // prepare reading offset - ignoring 0x
            case FTRACE_IGNORE_ZERO:
                parsingMode=FTRACE_IGNORE_X;
                break;
            case FTRACE_IGNORE_X:
                parsingMode=FTRACE_READ_OFFSET;
                break;

            // read offset until a '/' appears
            case FTRACE_READ_OFFSET:
                if (inputBuffer[position] == '/') {
                    parsingMode=FTRACE_FIND_BASE_ADDR;
                } else {
                    offset = offset * 16
                             + hexCharToDec[(int) inputBuffer[position]];
                }
                break;

            // wait until address starts, i.e. we encounter a '<'
            case FTRACE_FIND_BASE_ADDR:
                if (inputBuffer[position] == '<') {
                    address = 0;
                    parsingMode=FTRACE_READ_BASE_ADDR;
                }
                break;
            // same for the address of the caller function
            case FTRACE_FIND_CALLER_BASE_ADDR:
                if (inputBuffer[position] == '<') {
                    address = 0;
                    parsingMode=FTRACE_READ_CALLER_BASE_ADDR;
                }
                break;

            // read base or caller address until '>', convert it and add to list
            case FTRACE_READ_BASE_ADDR:
            case FTRACE_READ_CALLER_BASE_ADDR:
                if (inputBuffer[position] == '>') {
                    // add only direct function calls to ftrace's ignore list
                    if (addAddr(address - offset) && parsingMode == 8) {
                        ignoreFunc(callerBuffer, callerSize);
                    }
                    // reset
                    offset = 0;
                    // reset function name
                    callerSize = 0;
                    // go on
                    parsingMode ++;
                } else if ((j = hexCharToDec[(int) inputBuffer[position]])
                           != -1) {
                    address = address * 16 + j;
                } else {
                    // ignore ' <-'
                    if (parsingMode == FTRACE_READ_CALLER_BASE_ADDR
                        && inputBuffer[position] == '-') {
                        parsingMode = FTRACE_FIND_CALLER_BASE_ADDR;
                    } else {
                        parsingMode = FTRACE_DROP;
                    }
                }
                break;

            // ignore the rest of the line
            default:
                if (inputBuffer[position] == '\n') {
                    parsingMode = FTRACE_CHECK_COMMENT;
                }
            }
        }
    }
    return 0;
}
