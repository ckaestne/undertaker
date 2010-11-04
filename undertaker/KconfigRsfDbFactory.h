// -*- mode: c++ -*-
#ifndef kconfigrsfdbfactory_h__
#define kconfigrsfdbfactory_h__

#include <string>
#include <map>

#include "KconfigRsfDb.h"

typedef std::map<std::string, KconfigRsfDb*> ModelContainer;

class KconfigRsfDbFactory : protected ModelContainer {
public:
    static void loadModels();
    static void loadModels(std::string); //for arch-specific analysis
    static void loadWhitelist(const char *file);
    static KconfigRsfDb *lookupModel(const char *arch);
    static KconfigRsfDbFactory *getInstance();
    bool empty();

    iterator begin() { return ModelContainer::begin(); }
    const_iterator begin() const { return ModelContainer::begin(); }
    iterator end() { return ModelContainer::end(); }
    const_iterator end() const { return ModelContainer::end(); }
    size_type size() const { return ModelContainer::size(); }

private:
    KconfigRsfDb *registerRsfFile(const char* filename, std::string arch);
    ~KconfigRsfDbFactory();
};

#endif
