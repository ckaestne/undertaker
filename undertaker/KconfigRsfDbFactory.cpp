#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <pstreams/pstream.h>
#include <utility>
#include <fstream>

#include "KconfigRsfDbFactory.h"
#include "KconfigWhitelist.h"

void KconfigRsfDbFactory::loadWhitelist(const char *file) {
    KconfigRsfDbFactory *f = getInstance();
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

void KconfigRsfDbFactory::loadModels() {
    KconfigRsfDbFactory *f = getInstance();
    int found_models = 0;
    typedef std::list<std::string> FilenameContainer;
    FilenameContainer filenames;

    const boost::regex r("^kconfig-([[:alnum:]]+)\\.rsf$", boost::regex::perl);

    for (boost::filesystem::directory_iterator dir(".");
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
                f->registerRsfFile(filename->c_str(), found_arch);
                found_models++;

                std::cout << "I: loaded rsf model for " << found_arch
                          << std::endl;
            }
        }
    }

    if (found_models > 0) {
        std::cout << "I: found " << found_models << " rsf models" << std::endl;
    } else {
        std::cerr << "E: could not find any rsf models" << std::endl;
        exit(-1);
    }
}

bool KconfigRsfDbFactory::empty() {
    return ModelContainer::empty();
}
    
void KconfigRsfDbFactory::loadModels(std::string arch) {
    KconfigRsfDbFactory *f = getInstance();
    std::string filename = "kconfig-" + arch + ".rsf";

    /* Check if the RSF file exists */
    if (boost::filesystem::exists(filename)) {
    ModelContainer::iterator a = f->find(arch);

    if (a == f->end())
        f->registerRsfFile(filename.c_str(), arch);
        std::cout << "I: Loaded Model for architecture " << arch << std::endl;
    } else {
        std::cerr << "E: could not find rsf file for arch "
                  << arch << std::endl;
        exit(-1);
    }
}

KconfigRsfDb *KconfigRsfDbFactory::registerRsfFile(const char *filename, std::string arch) {
    std::ifstream rsf_file(filename);
    static std::ofstream devnull("/dev/null");

    if (!rsf_file.good()) {
    std::cerr << "could not open file for reading: "
          << filename << std::endl;
    return NULL;
    }
    KconfigRsfDb *db = new KconfigRsfDb(rsf_file, devnull);
    db->initializeItems();
    this->insert(std::make_pair(arch,db));
    return db;
};

KconfigRsfDb *KconfigRsfDbFactory::lookupModel(const char *arch)  {
    KconfigRsfDbFactory *f = getInstance();
    // first step: look if we have it in our models list;
    ModelContainer::iterator a = f->find(arch);
    if (a != f->end()) {
    // we've found it in our map, so return it
    return a->second;
    } else {
    // not found, is there an matching rsf around?
        const boost::regex r("^kconfig-([[:alnum:]]+)\\.rsf$", boost::regex::perl);

        /* Walk the directory */
        for (boost::filesystem::directory_iterator dir(".");
             dir != boost::filesystem::directory_iterator();
             ++dir) {
            boost::match_results<const char*> what;

            /* Current iterator filename */
            std::string filename = dir->path().filename();

            if (boost::regex_search(filename.c_str(), what, r)) {
        std::string found_arch = what[1];

        if (found_arch.compare(arch) == 0) {
            KconfigRsfDb *db = f->registerRsfFile(filename.c_str(), found_arch);
            return db;
        }
        }
    }
    return NULL;
    }
}

const char *KconfigRsfDbFactory::lookupArch(const KconfigRsfDb *model) {
    KconfigRsfDbFactory *f = getInstance();
    ModelContainer::iterator i;
    for (i = f->begin(); i != f->end(); i++) {
        if ((*i).second == model)
            return (*i).first.c_str();
    }
    return NULL;
}

KconfigRsfDbFactory *KconfigRsfDbFactory::getInstance() {
    static KconfigRsfDbFactory *instance;
    if (!instance) {
        instance = new KconfigRsfDbFactory();
    }
    return instance;
}

KconfigRsfDbFactory::~KconfigRsfDbFactory() {
    ModelContainer::iterator i;
    for (i = begin(); i != end(); i++)
    free((*i).second);
}
