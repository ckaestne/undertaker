#include <iostream>
#include <string>
#include "SatChecker.h"
#include "LimBoole.h"

SatChecker::SatChecker(const char *sat) : _sat(std::string(sat)), _parsed_sat(LimBoole::parse(sat)) {}
SatChecker::SatChecker(const std::string sat) : _sat(sat), _parsed_sat(LimBoole::parse(sat.c_str())) {}
SatChecker::~SatChecker() {
    LimBoole::release(const_cast<LimBoole::Sat*>(_parsed_sat));
}

bool SatChecker::operator()() throw (SatCheckerError) {
    bool result = LimBoole::check(const_cast<LimBoole::Sat*>(_parsed_sat), true);
    if (_parsed_sat->token == LimBoole::ERROR)  {
        std::cout << "Syntax error" << std::endl;
        throw SatCheckerError(_sat.c_str());
    }
    return result;
}

std::string SatChecker::pprint() {
    return LimBoole::pretty_print(const_cast<LimBoole::Sat*>(_parsed_sat));
}

#ifdef _TEST
void test(std::string s) {
    SatChecker checker(s);
    const char *t = "satisfiable";
    const char *f = "unsatisfiable";
    const char *a = checker() ? t:f;
    std::cout << checker.pprint() << " is " << a
	      << std::endl;
}
int main() {

    test("A & B");
    test("A & !A");
    test("B6 & (B5 <-> !S390) & (B6 <-> !B5) & !S390");
    try {
        test("a >,,asd/.sa,;.ljkxf;vnbmkzjghrarjkf.dvd,m.vjkb54y98g4tjoij");
    } catch (SatCheckerError &e) {
        std::cout << e.what() << std::endl;
    }
    return 0;
};
#endif
