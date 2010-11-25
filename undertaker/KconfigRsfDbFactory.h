// -*- mode: c++ -*-
#ifndef kconfigrsfdbfactory_h__
#define kconfigrsfdbfactory_h__

#include <string>
#include <map>

#include "ConfigurationModel.h"

typedef std::map<std::string, ConfigurationModel*> ModelContainer;

class KconfigRsfDbFactory : protected ModelContainer {
public:
    static void loadModels(std::string);
    static void loadModels(std::string, std::string); //for arch-specific analysis
    static void loadWhitelist(const char *file);
    static ConfigurationModel *lookupModel(const char *arch);
    static const char *lookupArch(const ConfigurationModel *model);
    static KconfigRsfDbFactory *getInstance();
    bool empty();

    iterator begin() { return ModelContainer::begin(); }
    const_iterator begin() const { return ModelContainer::begin(); }
    iterator end() { return ModelContainer::end(); }
    const_iterator end() const { return ModelContainer::end(); }
    size_type size() const { return ModelContainer::size(); }

private:
    ConfigurationModel *registerModelFile(std::string filename, std::string arch);
    ~KconfigRsfDbFactory();
};

#endif
