// -*- mode: c++ -*-
#ifndef modelcontainer_h__
#define modelcontainer_h__

#include <string>
#include <map>

#include "ConfigurationModel.h"

/**
 * \brief Container that maps ConfigurationModel classes to its architectures
 *
 * This class is basically a singleton that derives from
 * std::map<std::string, ConfigurationModel*>. It provides a few
 * convenience methods for model loading and lookups.
 */
class ModelContainer : public std::map<std::string, ConfigurationModel*> {
public:
    static void loadModels(std::string modeldir); ///< load models from the given directory
    static void loadModels(std::string, std::string); ///< for arch-specific analysis
    static void loadWhitelist(const char *file);
    static ConfigurationModel *lookupModel(const char *arch);
    static const char *lookupArch(const ConfigurationModel *model);
    static ModelContainer *getInstance();

private:
    ConfigurationModel *registerModelFile(std::string filename, std::string arch);
    ~ModelContainer();
};

#endif
