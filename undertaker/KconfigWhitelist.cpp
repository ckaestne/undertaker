#include "KconfigWhitelist.h"

bool KconfigWhitelist::isWhitelisted(const char *item) const {
    KconfigWhitelist::const_iterator it;

    for (it = begin(); it != end(); it++)
        if((*it).compare(item) == 0)
            return true;
    return false;
}

void KconfigWhitelist::addToWhitelist(const char *item) {
    if(!isWhitelisted(item))
        push_back(std::string(item));
}

const char *KconfigWhitelist::containsWhitelistedItem(const char *text) const {
    KconfigWhitelist::const_iterator it;
    std::string t(text);

    for (it = begin(); it != end(); it++)
        if ((t.find((*it)) != std::string::npos))
            return (*it).c_str();
    return NULL;
}

KconfigWhitelist *KconfigWhitelist::getInstance() {
    static KconfigWhitelist *instance;
    if (!instance) {
        instance = new KconfigWhitelist();
    }
    return instance;
}
