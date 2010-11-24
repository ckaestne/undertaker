// -*- mode: c++ -*-
#ifndef kconfigrsfdb_h__
#define kconfigrsfdb_h__

#include <string>
#include <map>
#include <deque>
#include <set>

#include "RsfReader.h"

class KconfigRsfDb : protected RsfReader {
public:
    KconfigRsfDb(std::ifstream &in, std::ostream &log);

    int doIntersect(const std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing, int &slice) const;

protected:
    void findSetOfInterestingItems(std::set<std::string> &working) const;
};

#endif
