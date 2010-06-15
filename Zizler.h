#ifndef ZIZLER_H_
#define ZIZLER_H_

#include <string>

typedef enum {
    Short,
    Medium,
    Long
} Mode;

bool ziztest(std::string file, Mode mode);

#endif /* ZIZLER_H_ */
