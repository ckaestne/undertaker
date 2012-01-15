/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <utility>
#include <fstream>

#include "ModelContainer.h"
#include "KconfigWhitelist.h"
#include "Logging.h"

ConfigurationModel* ModelContainer::loadModels(std::string model) {
    ModelContainer *f = getInstance();
    int found_models = 0;
    typedef std::list<std::string> FilenameContainer;
    FilenameContainer filenames;
    ConfigurationModel *ret = 0;

    const boost::regex model_regex("^([[:alnum:]]+)\\.model$", boost::regex::perl);

    if (! boost::filesystem::exists(model)){
        logger << error << "model '" << model << "' doesn't exist (neither directory nor file)" << std::endl;
        return 0;
    }

    if (! boost::filesystem::is_directory(model)) {
        /* A model file was specified, so load exactly this one */
        boost::match_results<const char*> what;
        std::string model_name = std::string(basename(model.c_str()));

        /* Strip the .model suffix if possible */
        if (boost::regex_search(model_name.c_str(), what, model_regex)) {
            model_name = std::string(what[1]);
        }
        ret = f->registerModelFile(model, model_name);
        logger << info << "loaded rsf model for " << model_name
               << std::endl;

        return ret;
    }


    for (boost::filesystem::directory_iterator dir(model);
         dir != boost::filesystem::directory_iterator();
         ++dir) {
#if !defined(BOOST_FILESYSTEM_VERSION) || BOOST_FILESYSTEM_VERSION == 2
        filenames.push_back(dir->path().filename());
#else
        filenames.push_back(dir->path().filename().string());
#endif
    }
    filenames.sort();

    for (FilenameContainer::iterator filename = filenames.begin();
         filename != filenames.end(); filename++) {
        boost::match_results<const char*> what;

        if (boost::regex_search(filename->c_str(), what, model_regex)) {
            std::string found_arch = what[1];
            ModelContainer::iterator a = f->find(found_arch);

            if (a == f->end()) {
                ConfigurationModel *a = f->registerModelFile(model + "/" + filename->c_str(), found_arch);
                /* overwrite the return value */
                if (a) ret = a;
                found_models++;

                logger << info << "loaded rsf model for " << found_arch
                       << std::endl;
            }
        }
    }

    if (found_models > 0) {
        logger << info << "found " << found_models << " rsf models" << std::endl;
        return ret;
    } else {
        logger << error << "could not find any models" << std::endl;
        return 0;
    }
}

// parameter filename will look like: 'models/x86.model', string like 'x86'
ConfigurationModel *ModelContainer::registerModelFile(std::string filename, std::string arch) {
    ConfigurationModel *db;

    /* Was already loaded */
    if ((db = lookupModel(arch.c_str()))) {
        logger << info << "A model for " << arch << " was already loaded" << std::endl;
        return db;
    }

    std::ifstream *model = new std::ifstream(filename.c_str());
    if (!model->good()) {
        logger << error << "could not open file for reading: " << filename << std::endl;
        delete model;
        return NULL;
    }

    std::string::size_type i = filename.find(".model");
    if (i == std::string::npos)
        return NULL;

    filename.erase(i); filename.append(".rsf");
    std::ifstream *rsf = new std::ifstream(filename.c_str());
    if (!rsf->good()) {
        logger << warn << "could not open file for reading: "      << filename << std::endl;
        logger << warn << "checking the type of symbols will fail" << std::endl;
        rsf = new std::ifstream("/dev/null");
    }

    db = new ConfigurationModel(arch, model, rsf);

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
        logger << error << "Could not specify main model "
               << main_model << ", because no such model is loaded" << std::endl;
        return;
    }
    logger << info << "Using " << main_model << " as primary model" << std::endl;
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
    for (ModelContainer::iterator i = begin(); i != end(); i++)
        free((*i).second);
}
