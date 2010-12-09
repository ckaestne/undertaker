// -*- mode: c++ -*-
#ifndef rsfreader_h__
#define rsfreader_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>


class RsfReader : public std::map<std::string, std::deque<std::string> > {
public:

    RsfReader(std::ifstream &f, std::ostream &log=std::cout);

    const std::string *getValue(const std::string &key) const;
    void print_contents(std::ostream &out);

private:
    std::deque<std::string> parse(const std::string& line);
    void read_rsf(std::ifstream &rsf_file);
    std::ostream &log;
};

#endif
