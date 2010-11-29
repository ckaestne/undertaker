#include "KconfigWhitelist.h"

bool KconfigWhitelist::isWhitelisted(const char *item) const {
    KconfigWhitelist::const_iterator it;

    for (it = begin(); it != end(); it++)
        if((*it).compare(item) == 0)
            return true;
    return false;
}

void KconfigWhitelist::addToWhitelist(const std::string item) {
    if(!isWhitelisted(item.c_str()))
        push_back(item);
}

KconfigWhitelist *KconfigWhitelist::getInstance() {
    static KconfigWhitelist *instance;
    if (!instance) {
        instance = new KconfigWhitelist();
    }
    return instance;
}
