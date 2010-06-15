#include "Zizler.h"
#include "Ziz.h"

#include <cassert>
#include <iostream>

bool ziztest(std::string file, Mode mode)
{
    std::cerr << "Testing " << file << std::endl;
    Ziz::Parser ziz;
    Ziz::CPPFile cppfile;
    try {
	cppfile = ziz.Parse(file);
    } catch(Ziz::ZizException& e) {
	std::cerr << "caught ZizException: " << e.what() << std::endl;
	return false;
    } catch(...) {
	std::cerr << "caught exception" << std::endl;
	return false;
    }
    if (mode == Short) {
	std::cout + cppfile;
    } else if (mode == Medium) {
	std::cout << cppfile;
    } else if (mode == Long) {
	std::cout >> cppfile;
    } else {
	assert(false); // shall never happen
	return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0])
	          << " [-s|--short|-l|--long] FILE [FILES]" << std::endl;
        return 0;
    }

    Mode mode = Medium;
    int fileArg = 1;
    std::string arg(argv[1]);
    if (arg.compare("-l") == 0 || arg.compare("--long") == 0) {
	fileArg++;
	mode = Long;
    } else if (arg.compare("-s") == 0 || arg.compare("--short") == 0) {
	fileArg++;
	mode = Short;
    }

    int fail = 0;
    for (int i = fileArg; i < argc; i++) {
        std::string file(argv[i]);
        if (ziztest(file, mode)) {
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
