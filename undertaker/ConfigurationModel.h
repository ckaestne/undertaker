// -*- mode: c++ -*-
#ifndef configuration_model_h__
#define configuration_model_h__

#include <string>
#include <map>
#include <deque>
#include <set>

#include "RsfReader.h"

class ConfigurationModel : public RsfReader {
public:
    ConfigurationModel(std::ifstream &in, std::ostream &log);

    int doIntersect(const std::set<std::string> myset, std::ostream &out, std::set<std::string> &missing) const;
    void findSetOfInterestingItems(std::set<std::string> &working) const;
    static std::string getMissingItemsConstraints(std::set<std::string> &missing);
};

#endif
