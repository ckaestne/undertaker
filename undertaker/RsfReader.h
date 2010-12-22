// -*- mode: c++ -*-
#ifndef rsfreader_h__
#define rsfreader_h__

#include <deque>
#include <map>
#include <string>
#include <iostream>


typedef std::deque<std::string> StringList;
typedef std::map<std::string, StringList> RsfMap;

/**
 * \brief Reads RSF files
 */
class RsfReader : public RsfMap {
public:

    RsfReader(std::ifstream &f, std::ostream &log=std::cout);

    const std::string *getValue(const std::string &key) const;
    void print_contents(std::ostream &out);

private:
    StringList parse(const std::string& line);
    void read_rsf(std::ifstream &rsf_file);
    std::ostream &log;
};

#endif
