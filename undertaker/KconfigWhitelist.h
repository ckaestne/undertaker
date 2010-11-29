// -*- mode: c++ -*-
#ifndef kconfigwhitelist_h__
#define kconfigwhitelist_h__

#include <list>
#include <string>

struct KconfigWhitelist : protected std::list<std::string> {
    static KconfigWhitelist *getInstance();
    bool empty();
    bool isWhitelisted(const std::string) const;
    void addToWhitelist(const std::string);
private:
    KconfigWhitelist() : std::list<std::string>() {} // private c'tor
};

#endif
