// -*- mode: c++ -*-
#ifndef kconfigwhitelist_h__
#define kconfigwhitelist_h__

#include <list>
#include <string>

struct KconfigWhitelist : protected std::list<std::string> {
    static KconfigWhitelist *getInstance();
    bool empty();
    bool isWhitelisted(const char *item) const;
    void addToWhitelist(const char *item);
    const char *containsWhitelistedItem(const char *text) const;
private:
    KconfigWhitelist() : std::list<std::string>() {} // private c'tor
};

#endif
