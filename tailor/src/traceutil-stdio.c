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

#include "traceutil-stdio.h"

// information about loaded kernel modules
module loadedModule[MODULESIZE];
int loadedModules = 0;
unsigned long long moduleBaseAddr;

// path information for required files
const char * modulePath;
const char * tracePath;
const char * ignorePath;
const char * outputPath;

// file pointers of input and output
FILE * outputFile;
FILE * ignoreFile;

int ignoreFileReopenUpperBound = IGNOREFILEREOPENUPPERBOUND;
const int ignoreFileReopenLimit = IGNOREFILEREOPENLOWERBOUND;
int ignoreFileReopenCounter = 0;

/* This function reloads Linux's module table. For every module found in the
 * table, we store its name, the base address and its size. By doing so, we can
 * later test whether a given address is inside a certain module's address range
 */

inline void readModules() {
    FILE * moduleFile;
    moduleFile = fopen (modulePath,"r");
    // On error, use old version of procmodules
    if(moduleFile == NULL) {
        return;
    }
    loadedModules = 0;
    moduleBaseAddr = 0;
    while (loadedModules < MODULESIZE) {
        module * m = &loadedModule[loadedModules++];
        // Scan a line from the modules output & store every entry in the table
        int scanArguments = fscanf (moduleFile,"%" STR(MODULENAMELENGTH)
                "s %30llu %*20d %*1024s %*100s 0x%" STR(ADDRLENGTH)
                "llx%*1000[^\n]\n",m->name,&m->length,&m->base);
        if (scanArguments == EOF) {
            break;
        } else if (moduleBaseAddr == 0 || moduleBaseAddr > m->base) {
            moduleBaseAddr = m->base;
        }
    }
    fclose(moduleFile);
}

/* This function adds a found address to the output. It also adds the address to
 * the internal hash map, so that we can filter out already found addresses.  If
 * the given address was located inside an LKM or we didn't scan the module
 * table yet, a call to addModuleAddr(addr) is triggered.
 */

inline bool addAddr(unsigned long long addr) {
    // address new?
    if (hashadd(addr)) {
        // module address (if modules enabled)?
        if (modulePath != NULL && addr >= moduleBaseAddr) {
            if (addModuleAddr(addr)) {
                return true;
            } else {
                // if not found, read list and check again
                readModules();
                if (addModuleAddr(addr)) {
                    return true;
                }
            }
        }
        // vmlinux address
        fprintf(outputFile,"%llx\n",addr);
        return true;
    }
    return false;
}

/* This function adds an address from an LKM to the output. Therefore we iterate
 * the table of modules, and check if the address is between the lower and the
 * upper bound of any module. If we find the module, we return *true* to signal
 * success. If we don't find any module matching, we return *false*; this will
 * tell the caller to call readModules() and therefore rescan the module table
 * currently present.
 */

inline bool addModuleAddr(unsigned long long addr) {
    int currentModule = 0;
    // Look in every module
    while (currentModule < MODULESIZE) {
        module m = loadedModule[currentModule++];
        unsigned long long lowAddr = m.base;
        unsigned long long highAddr = lowAddr + m.length;
        // If the address is inside the range of a module
        if (addr>=lowAddr && addr<=highAddr) {
            unsigned long long relativeAddr = addr-lowAddr;
            // output the offset and the module's name
            fprintf(outputFile,"%llx %s\n",relativeAddr, m.name);
            return true;
        }
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

inline void ignoreFunc(char *name) {
    // Otherwise, we add the function's name to the ignore list.
    if (ignoreFile == NULL
        || ++ignoreFileReopenCounter >= ignoreFileReopenUpperBound) {
        ignoreFileReopenUpperBound /= 2;
        if (ignoreFileReopenUpperBound < ignoreFileReopenLimit) {
            ignoreFileReopenUpperBound = ignoreFileReopenLimit;
        }
        ignoreFileReopenCounter = 0;

        fclose(ignoreFile);
        ignoreFile = fopen (ignorePath,"a");
    }
    fprintf(ignoreFile,"%s\n",name);
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
 */

int main ( int argc, const char* argv[] ) {
    char name[1024];
    // Offset of injected mcount function call
    unsigned long long baseOffset;
    // Address where the mcount function was called
    //   => address - offset = starting address of function
    unsigned long long baseAddr;
    // Address from where the current function was called
    unsigned long long srcAddr;

    FILE * traceFile;

    if (argc < 4) {
        printf("Usage %s <trace> <output> <ignore> [<module>]\n",argv[0]);
        return -1;
    }
    tracePath = argv[1];
    outputPath = argv[2];
    ignorePath = argv[3];

    // optional module support
    if (argc < 5) {
        modulePath = NULL;
    } else {
        modulePath=argv[4];
        readModules();
    }

    outputFile = fopen(outputPath,"a");
    traceFile = fopen (tracePath,"r");
    ignoreFile = fopen (ignorePath,"a");
    if(!outputFile || !traceFile || !ignoreFile) {
        fprintf(stderr, "Error opening required files, exiting...");
        return 1;
    }

    while (1) {
        // Scan a line from the trace pipe
        int scanArguments = fscanf (traceFile,"\n%*60[^: ]: %100[^+ ]"
                "+0x%20llx/%*[^<]<%20llx>",name,&baseOffset,&baseAddr);
        if (scanArguments == EOF) {
            fclose(traceFile);
            return 0;
        } else if (scanArguments >= 3) {
            // Try to add address to hashset
            if (addAddr(baseAddr-baseOffset)) {
                ignoreFunc(name);
            }

            // Also get information, where the current function was called from.
            if (fscanf(traceFile," <-%*" STR(FUNCNAMELENGTH) "[^<\n]<%"
                       STR(ADDRLENGTH) "llx>",&srcAddr) == 1) {
                addAddr(srcAddr);
            }
        }
    }
}
