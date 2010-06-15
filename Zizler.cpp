#include "Zizler.h"
#include "Ziz.h"

#include <iostream>

bool ziztest(std::string file)
{
    std::cerr << "Testing " << file << std::endl;
    Ziz::Parser ziz;
    Ziz::CPPFile cppfile = ziz.Parse(file);
    std::cout << cppfile;
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0]) << " FILE [FILES]"
                  << std::endl;
        return 0;
    }

    int fail = 0;
    for (int i = 1; i < argc; i++) {
        std::string file(argv[i]);
        if (ziztest(file)) {
            std::cerr << file << " ok" << std::endl;
        } else {
            std::cerr << file << " failed" << std::endl;
            fail++;
        }
    }

    if (fail == 0) {
        std::cerr << "All tests passed." << std::endl;
    } else {
        std::cerr << "Some tests failed." << std::endl;
    }
    return fail;
}
