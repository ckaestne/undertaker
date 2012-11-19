#ifndef TRACEUTIL_CONST_H
#define TRACEUTIL_CONST_H

// Buffersize for reading blocks
#define INPUTBUFFERSIZE 16384

// Buffersize for function name
#define FUNCNAMELENGTH 200

// Charbuffer for converting long to printable
#define ADDRLENGTH 20

// Maximum length of a name in /proc/modules
#define MODULENAMELENGTH 100

// Maximum number of loaded modules (for buffer size)
#define MODULESIZE 1000

// First flush after n lines
#define IGNOREFILEREOPENUPPERBOUND 1024

// Minimum lines to flush
#define IGNOREFILEREOPENLOWERBOUND 8

#endif
