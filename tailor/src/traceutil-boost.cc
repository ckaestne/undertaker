// Compile with $ g++ -std=c++0x foo.cpp -lboost_regex && ./a.out

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <boost/regex.hpp>
#include <unordered_set>
#include "constants.h"

using namespace boost;
using namespace std;

// information about loaded kernel modules.
typedef struct module {
    string name, parent, status;
    unsigned long long base, length;
    unsigned int usage;
} module;

// the addresses already encountered
unordered_set<unsigned long long> calledAddr;
// our view of the currently loaded modules
map<unsigned long long,module> loadedModules;

// paths to required input files
const char * modulePath;
const char * tracePath;
const char * ignorePath;
const char * outputPath;

// input and output file streams
ofstream outputFile;
ofstream ignoreFile;

int ignoreFileReopenUpperBound = IGNOREFILEREOPENUPPERBOUND;
const int ignoreFileReopenLimit = IGNOREFILEREOPENLOWERBOUND;
int ignoreFileReopenCounter = 0;

/* This function reloads Linux's module table. For every module found in the
 * table, we store the information given by the module input file (usually
 * /proc/modules). By doing so, we can later test whether a given address is
 * inside a certain module's address range.
 */

void readModules() {
    string moduleLine;
    ifstream moduleFile;
    loadedModules.clear();
    moduleFile.open(modulePath, ios::in);
    while(moduleFile.good()) {
        module item;
        moduleFile >> item.name >> dec >> item.length >> item.usage
                >> item.parent >> item.status >> hex >> item.base;
        moduleFile.ignore(1000,10); // skip rest of line
        if (item.name.size()>0) {
            loadedModules.insert(
                    pair<unsigned long long,module>(item.base,item));
        }
    }
    moduleFile.close();
}

/* This function adds an address from an LKM to the output. Therefore we iterate
 * the table of modules, and check if the address is between the lower and the
 * upper bound of any module. If we find the module, we return *true* to signal
 * success. If we don't find any module matching, we return *false*; this will
 * tell the caller to call readModules() and therefore rescan the module table
 * currently present.
 */

bool addModuleAddr(unsigned long long addr) {
    for (map<unsigned long long,module>::iterator it=loadedModules.begin() ;
         it != loadedModules.end(); it++ ) {
        unsigned long long lowAddr = (*it).first;
        unsigned long long highAddr = lowAddr + (*it).second.length;
        if (addr>=lowAddr && addr<=highAddr) {
            unsigned long long relativeAddr = addr-lowAddr;
            outputFile << relativeAddr << " " <<  (*it).second.name << endl;
            return true;
        }
    }
    return false;
}

/* This function adds a found address to the output. It also adds the address to
 * the internal hash map, so that we can filter out already found addresses.  If
 * the given address was located inside an LKM we do not know about yet or we
 * didn't scan the module table yet, a call to addModuleAddr(addr) is triggered.
 */

bool addAddr(unsigned long long addr) {
    // address new?
    if (calledAddr.insert(addr).second) {
        // module address (if modules enabled)?
        if (modulePath != NULL && addr >= loadedModules.begin()->first) {
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
        outputFile << addr << endl;
        return true;
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

void ignoreFunc(string name) {
    // Otherwise, we add the function's name to the ignore list.
    if (ignoreFile == NULL
        || ++ignoreFileReopenCounter >= ignoreFileReopenUpperBound) {
        ignoreFileReopenUpperBound /= 2;
        if (ignoreFileReopenUpperBound < ignoreFileReopenLimit) {
            ignoreFileReopenUpperBound = ignoreFileReopenLimit;
        }
        ignoreFileReopenCounter = 0;

        ignoreFile.close();
        ignoreFile.open(ignorePath,ios::app);
    }
    ignoreFile << name << endl;
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
 * We store all this information in the loadedModule set.
 * To determine if the address read from the ftrace output is inside a
 * module or not, we use the lowest base address from the modules output as a
 * discriminator (if the address is bigger than the lowest base address, it is
 * considered part of an LKM). On the x86 systems we tested, the lowest LKM
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

int main( int argc, const char* argv[] ) {
    if (argc < 4) {
        cout << "Usage "<< argv[0] << " <trace> <output> <ignore> [<module>]"
                << endl;
        return -1;
    }
    tracePath = argv[1];
    outputPath = argv[2];
    ignorePath = argv[3];

    // optional module support
    if (argc < 5) {
        modulePath = NULL;
    }
    else {
        modulePath=argv[4];
        readModules();
    }

    outputFile.open(outputPath, ios::trunc);
    outputFile << hex;

    // prepare regex
    string traceLine;
    ifstream traceFile;

    ignoreFile.open(ignorePath,ios::app);
    const regex rgx("^[^#][^\n]*?: ([^+\n]+)[+]0x([0-9a-f]+)/[^<\n]*"
                "<([0-9a-f]+)>(?: <-[^<\n]*<([0-9a-f]+)>)?$");
    smatch match;
    traceFile.open(tracePath, ios::in);
    while(!traceFile.eof()) {
        getline(traceFile, traceLine);
        if (regex_search (traceLine,match,rgx)) {
            string name = match[1];

            unsigned long long baseOffset;
            sscanf(match[2].str().c_str(),"%llx",&baseOffset);

            unsigned long long baseAddr;
            sscanf(match[3].str().c_str(),"%llx",&baseAddr);

            // if new --> ignore now
            if (addAddr(baseAddr-baseOffset)) {
                ignoreFunc(name);
            }

            // if calling source address given, use this too
            if (match.size() >= 5) {
                unsigned long long srcAddr;
                sscanf(match[4].str().c_str(),"%llx",&srcAddr);
                addAddr(srcAddr);
            }
        }
    }
    traceFile.close();
    outputFile.close();
    return 0;
}
