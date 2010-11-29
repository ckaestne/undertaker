// -*- mode: c++ -*-
#ifndef kconfigwhitelist_h__
#define kconfigwhitelist_h__

#include <list>
#include <string>

/**
 * \brief Manages the whitelist of Kconfig Items
 */
struct KconfigWhitelist : protected std::list<std::string> {
    static KconfigWhitelist *getInstance(); //!< accessor for this singleton
    bool empty(); //!< checks if the whitelist is empty
    bool isWhitelisted(const char *) const; //!< checks if the given item is in the whitelist
    void addToWhitelist(const std::string); //!< adds an item to the whitelist
private:
    KconfigWhitelist() : std::list<std::string>() {} //!< private c'tor
};

#endif
