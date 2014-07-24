/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#ifdef DEBUG
#define BOOST_FILESYSTEM_NO_DEPRECATED
#endif

#include "ModelContainer.h"
#include "RsfConfigurationModel.h"
#include "CnfConfigurationModel.h"
#include "Logging.h"

#include <boost/regex.hpp>
#include <boost/filesystem.hpp>


static const boost::regex model_regex("^([-[:alnum:]]+)\\.(model|cnf)$");

ConfigurationModel* ModelContainer::loadModels(std::string model) {
    ModelContainer &f = getInstance();
    int found_models = 0;
    std::list<std::string> filenames;
    ConfigurationModel *ret = nullptr;

    if (! boost::filesystem::exists(model)){
        logger << error << "model '" << model << "' doesn't exist (neither directory nor file)" << std::endl;
        return nullptr;
    }
    if (! boost::filesystem::is_directory(model)) {
        /* A model file was specified, so load exactly this one */
        boost::filesystem::path p(model);
        std::string model_name = p.stem().string();

        ret = f.registerModelFile(model, model_name);
        if (ret) {
            logger << info << "loaded " << ret->getModelVersionIdentifier()
                   << " model for " << model_name << std::endl;
        } else {
            logger << error << "failed to load model from " << model << std::endl;
        }
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

    for (const std::string &filename : filenames) {
        boost::smatch what;
        if (boost::regex_search(filename, what, model_regex)) {
            std::string found_arch = what[1];

            // if there is no model for 'found_arch' in f, load it
            if (f.find(found_arch) == f.end()) {
                ConfigurationModel *mod = f.registerModelFile(model + "/" + filename, found_arch);
                /* overwrite the return value */
                if (mod) ret = mod;
                found_models++;

                logger << info << "loaded " << mod->getModelVersionIdentifier()
                       << " model for " << found_arch << std::endl;
            }
        }
    }
    if (found_models > 0) {
        logger << info << "found " << found_models << " models" << std::endl;
        return ret;
    } else {
        logger << error << "could not find any models" << std::endl;
        return nullptr;
    }
}

// parameter filename will look like: 'models/x86.model', string like 'x86'
ConfigurationModel *ModelContainer::registerModelFile(std::string filename, std::string arch) {
    ConfigurationModel *db = lookupModel(arch);
    /* Was already loaded */
    if (db != nullptr) {
        logger << info << "A model for " << arch << " was already loaded." << std::endl;
        return db;
    }
    boost::filesystem::path filepath(filename);
    if (filepath.extension() == ".cnf") {
        db = new CnfConfigurationModel(filename);
    } else {
        db = new RsfConfigurationModel(filename);
    }
    this->emplace(arch, db);
    return db;
};

ConfigurationModel *ModelContainer::lookupModel(const std::string &arch)  {
    ModelContainer &f = getInstance();
    // first step: look if we have it in our models list;
    auto a = f.find(arch);
    if (a != f.end()) {
        // we've found it in our map, so return it
        return a->second;
    } else {
        // No model was found
        return nullptr;
    }
}

const std::string ModelContainer::lookupArch(const ConfigurationModel *model) {
    for (const auto &entry : getInstance())  // pair<string, ConfigurationModel *>
        if (entry.second == model)
            return entry.first;

    return {};
}

ConfigurationModel *ModelContainer::lookupMainModel() {
    ModelContainer &f = getInstance();
    return ModelContainer::lookupModel(f.main_model);
}

void ModelContainer::setMainModel(std::string main_model) {
    ModelContainer &f = getInstance();

    if (!ModelContainer::lookupModel(main_model)) {
        logger << error << "Could not specify main model "
               << main_model << ", because no such model is loaded" << std::endl;
        return;
    }
    logger << info << "Using " << main_model << " as primary model" << std::endl;
    f.main_model = main_model;
}

const std::string &ModelContainer::getMainModel() {
    return getInstance().main_model;
}


ModelContainer &ModelContainer::getInstance() {
    static ModelContainer instance;
    return instance;
}

ModelContainer::~ModelContainer() {
    for (auto &entry : *this)  // pair<string, ConfigurationModel *>
        delete entry.second;
}
