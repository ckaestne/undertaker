#include "KconfigWhitelist.h"
#include "ModelContainer.h"

#include <fstream>
#include <boost/regex.hpp>

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

int KconfigWhitelist::loadWhitelist(const char *file) {
    ModelContainer *f = ModelContainer::getInstance();
    if (f->empty())
        return 0;

    std::ifstream whitelist(file);
    std::string line;
    const boost::regex r("^#.*", boost::regex::perl);

    int n = 0;

    while (std::getline(whitelist, line)) {
        boost::match_results<const char*> what;

        if (boost::regex_search(line.c_str(), what, r))
            continue;

        n++;
        this->addToWhitelist(line.c_str());
    }
    return n;
}

