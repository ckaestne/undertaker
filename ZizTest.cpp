#include "ZizTest.h"
#include "Ziz.h"

#include <iostream>

bool ZizTest::Test(std::string file)
{
    std::cout << "Testing " << file << std::endl;
    return true;
}


int main (int argc, char **argv)
{
    Ziz ziz;
    std::cout << ziz.GetSomeShit() << std::endl;
}
