#include <iostream>
#include <string>
#include "SatChecker.h"
#include "LimBoole.h"

SatChecker::SatChecker(const char *sat) : _sat(std::string(sat)) {
    _parsed_sat = LimBoole::parse(sat);
}

SatChecker::SatChecker(const std::string sat) : _sat(sat) {
    _parsed_sat = LimBoole::parse(sat.c_str());
}

SatChecker::~SatChecker() {
    LimBoole::release(const_cast<LimBoole::Sat*>(_parsed_sat));
}

bool SatChecker::operator()() throw (SatCheckerError) {
    if (_parsed_sat->token == LimBoole::ERROR)  {
        //        std::cout << "Syntax error" << std::endl;
        throw SatCheckerError(_sat.c_str());
    }
    bool result = LimBoole::check(const_cast<LimBoole::Sat*>(_parsed_sat), true);
    return result;
}

std::string SatChecker::pprint() {
    return LimBoole::pretty_print(const_cast<LimBoole::Sat*>(_parsed_sat));
}

#ifdef TEST_SatChecker
#include <assert.h>
#include <typeinfo>
#include "LimBoole.cpp"


bool test(std::string s, bool result, std::runtime_error *error = 0) {
    SatChecker checker(s);
    if (error == 0)  {
        if (checker() != result) {
            std::cerr << "FAILED: " << s << " should be " << result << std::endl;
        }
    }
    else {
        try {
            if (checker() != result) {
                std::cerr << "FAILED: " << s << " should be " << result << std::endl; 
            }
        } catch (std::runtime_error &e) {
            if (typeid(*error) != typeid(e)) {
                std::cerr << "FAILED: " << s << " didn't throw the right exception (Should " << typeid(*error).name() 
                          << ", is " << typeid(e).name() << ")" << std::endl;
            }
        }
    }
    std::cout << "PASSED: " << s << std::endl;
}

int main() {

    test("A & B", true);
    test("A & !A", false);
    test("(A -> B) -> A -> A", true);
    test("(A <-> B) & (B <-> C) & (C <-> A)", true);
    test("B6 & (B5 <-> !S390) & (B6 <-> !B5) & !S390", false);
    test("a >,,asd/.sa,;.ljkxf;vnbmkzjghrarjkf.dvd,m.vjkb54y98g4tjoij", false, &SatCheckerError(""));
    
    return 0;
};
#endif
