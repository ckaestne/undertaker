#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <utility>
#include <fstream>

#include "ModelContainer.h"
#include "KconfigWhitelist.h"

void ModelContainer::loadModels(std::string model) {
    ModelContainer *f = getInstance();
    int found_models = 0;
    typedef std::list<std::string> FilenameContainer;
    FilenameContainer filenames;

    const boost::regex model_regex("^([[:alnum:]]+)\\.model$", boost::regex::perl);

    if (! boost::filesystem::exists(model)){
        std::cerr << "E: model '" << model << "' doesn't exist (neither directory nor file)" << std::endl;
        exit(-1);
    }

    if (! boost::filesystem::is_directory(model)) {
        /* A model file was specified, so load exactly this one */
        boost::match_results<const char*> what;
        std::string model_name = std::string(basename(model.c_str()));

        /* Strip the .model suffix if possible */
        if (boost::regex_search(model_name.c_str(), what, model_regex)) {
            model_name = std::string(what[1]);
        }
        f->registerModelFile(model, model_name);
        std::cout << "I: loaded rsf model for " << model_name
                  << std::endl;

        return;
    }


    for (boost::filesystem::directory_iterator dir(model);
         dir != boost::filesystem::directory_iterator();
         ++dir) {
        filenames.push_back(dir->path().filename());
    }
    filenames.sort();

    for (FilenameContainer::iterator filename = filenames.begin();
         filename != filenames.end(); filename++) {
        boost::match_results<const char*> what;

        if (boost::regex_search(filename->c_str(), what, model_regex)) {
            std::string found_arch = what[1];
            ModelContainer::iterator a = f->find(found_arch);

            if (a == f->end()) {
                f->registerModelFile(model + "/" + filename->c_str(), found_arch);
                found_models++;

                std::cout << "I: loaded rsf model for " << found_arch
                          << std::endl;
            }
        }
    }

    if (found_models > 0) {
        std::cout << "I: found " << found_models << " rsf models" << std::endl;
    } else {
        std::cerr << "E: could not find any models" << std::endl;
        exit(-1);
    }
}

ConfigurationModel *ModelContainer::registerModelFile(std::string filename, std::string arch) {
    ConfigurationModel *db;
    /* Was already loaded */
    if ((db = lookupModel(arch.c_str()))) {
        std::cout << "I: A model for " << arch << " was already loaded" << std::endl;
        return db;
    }

    std::ifstream rsf_file(filename.c_str());
    static std::ofstream devnull("/dev/null");

    if (!rsf_file.good()) {
        std::cerr << "could not open file for reading: "
                  << filename << std::endl;
        return NULL;
    }
    db = new ConfigurationModel(rsf_file, devnull);

    this->insert(std::make_pair(arch,db));

    return db;
};

ConfigurationModel *ModelContainer::lookupModel(const char *arch)  {
    ModelContainer *f = getInstance();
    // first step: look if we have it in our models list;
    ModelContainer::iterator a = f->find(arch);
    if (a != f->end()) {
        // we've found it in our map, so return it
        return a->second;
    } else {
        // No model was found
        return NULL;
    }
}

const char *ModelContainer::lookupArch(const ConfigurationModel *model) {
    ModelContainer *f = getInstance();
    ModelContainer::iterator i;
    for (i = f->begin(); i != f->end(); i++) {
        if ((*i).second == model)
            return (*i).first.c_str();
    }
    return NULL;
}

ConfigurationModel *ModelContainer::lookupMainModel() {
    ModelContainer *f = getInstance();
    return ModelContainer::lookupModel(f->main_model.c_str());
}

void ModelContainer::setMainModel(std::string main_model) {
    ModelContainer *f = getInstance();
    if (!ModelContainer::lookupModel(main_model.c_str())) {
        std::cerr << "E: Could not specify main model " << main_model << ", because no such model is loaded" << std::endl;
        return;
    }
    std::cout << "I: Using " << main_model << " as primary model" << std::endl;
    f->main_model = main_model;
}

const char *ModelContainer::getMainModel() {
    ModelContainer *f = getInstance();
    return f->main_model.c_str();
}


ModelContainer *ModelContainer::getInstance() {
    static ModelContainer *instance;
    if (!instance) {
        instance = new ModelContainer();
    }
    return instance;
}

ModelContainer::~ModelContainer() {
    ModelContainer::iterator i;
    for (i = begin(); i != end(); i++)
        free((*i).second);
}
