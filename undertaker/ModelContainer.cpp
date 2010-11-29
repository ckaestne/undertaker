#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <pstreams/pstream.h>
#include <utility>
#include <fstream>

#include "ModelContainer.h"
#include "KconfigWhitelist.h"

void ModelContainer::loadWhitelist(const char *file) {
    ModelContainer *f = getInstance();
    if (f->empty())
        return;

    std::ifstream whitelist(file);
    std::string line;
    const boost::regex r("^#.*", boost::regex::perl);
    KconfigWhitelist *wl = KconfigWhitelist::getInstance();

    int n = 0;

    while (std::getline(whitelist, line)) {
        boost::match_results<const char*> what;

        if (boost::regex_search(line.c_str(), what, r))
            continue;

        n++;
        wl->addToWhitelist(line.c_str());
    }
    std::cout << "I: loaded " << n << " items to whitelist" << std::endl;
}

void ModelContainer::loadModels(std::string modeldir) {
    ModelContainer *f = getInstance();
    int found_models = 0;
    typedef std::list<std::string> FilenameContainer;
    FilenameContainer filenames;

    const boost::regex r("^([[:alnum:]]+)\\.model$", boost::regex::perl);

    if (! (boost::filesystem::exists(modeldir) && boost::filesystem::is_directory(modeldir))) {
        std::cerr << "E: model directory '" << modeldir << "' doesn't exist" << std::endl;
        exit(-1);
    }

    for (boost::filesystem::directory_iterator dir(modeldir);
         dir != boost::filesystem::directory_iterator();
         ++dir) {
        filenames.push_back(dir->path().filename());
    }
    filenames.sort();

    for (FilenameContainer::iterator filename = filenames.begin();
         filename != filenames.end(); filename++) {
        boost::match_results<const char*> what;

        if (boost::regex_search(filename->c_str(), what, r)) {
            std::string found_arch = what[1];
            ModelContainer::iterator a = f->find(found_arch);

            if (a == f->end()) {
                f->registerModelFile(modeldir + "/" + filename->c_str(), found_arch);
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


void ModelContainer::loadModels(std::string modeldir, std::string arch) {
    ModelContainer *f = getInstance();
    std::string filename = modeldir + "/" + arch + ".model";

    /* Check if the RSF file exists */
    if (boost::filesystem::exists(filename)) {
    ModelContainer::iterator a = f->find(arch);

    if (a == f->end())
        f->registerModelFile(filename, arch);
        std::cout << "I: Loaded rsf model for architecture " << arch << std::endl;
    } else {
        std::cerr << "E: could not find rsf file for arch "
                  << arch << std::endl;
        exit(-1);
    }
}

ConfigurationModel *ModelContainer::registerModelFile(std::string filename, std::string arch) {
    std::ifstream rsf_file(filename.c_str());
    static std::ofstream devnull("/dev/null");

    if (!rsf_file.good()) {
    std::cerr << "could not open file for reading: "
          << filename << std::endl;
    return NULL;
    }
    ConfigurationModel *db = new ConfigurationModel(rsf_file, devnull);

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
