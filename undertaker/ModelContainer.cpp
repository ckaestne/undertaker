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

#include <boost/filesystem.hpp>
#include <future>


// parameter filename will look like: 'models/x86.model', ext: 'model'
static ConfigurationModel *loadModelFile(const std::string &filename, const std::string &ext) {
    // return value cannot be nullptr! if allocation fails, a std::bad_alloc is thrown
    if (ext == ".cnf")
        return new CnfConfigurationModel(filename);
    else
        return new RsfConfigurationModel(filename);
}

ConfigurationModel* ModelContainer::loadModels(std::string model) {
    if (!boost::filesystem::exists(model)) {
        Logging::error("model '", model, "' doesn't exist (neither directory nor file)");
        return nullptr;
    }
    ModelContainer &f = getInstance();
    ConfigurationModel *ret = nullptr;

    // only one model file was specified, so load exactly this one
    if (!boost::filesystem::is_directory(model)) {
        boost::filesystem::path p(model);
        std::string found_arch = p.stem().string();

        if (f.find(found_arch) == f.end()) {
            ret = loadModelFile(model, p.extension().string());
            Logging::info("loaded ", ret->getModelVersionIdentifier(), " model for ", found_arch);
            f.emplace(found_arch, ret);
        }
        return ret;
    }
    // a directory was specified, parse all models within this directory (asynchronously)
    std::map<std::string, std::future<ConfigurationModel *>> futures;
    int found_models = 0;

    for (boost::filesystem::directory_iterator dir(model), end; dir != end; ++dir) {
        const boost::filesystem::path dir_entry = dir->path();
        const std::string ext = dir_entry.extension().string();
        if (ext == ".cnf" || ext == ".model") {
            const std::string found_arch = dir_entry.stem().string();
            futures.emplace(found_arch, std::async(std::launch::async, loadModelFile,
                                                   dir_entry.string(), ext));
        }
    }
    // collect all ConfigurationModel pointers, calculated by the futures
    for (auto &fut : futures) {
        // get() blocks until the future is finished
        ConfigurationModel *mod = fut.second.get();
        if (f.find(fut.first) == f.end()) {
            found_models++;
            ret = mod;
            Logging::info("loaded ", mod->getModelVersionIdentifier(), " model for ", fut.first);
            f.emplace(fut.first, mod);
        }
    }
    if (found_models > 0) {
        Logging::info("found ", found_models, " models");
        return ret;
    } else {
        Logging::error("could not find any models");
        return nullptr;
    }
}

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
    return ModelContainer::lookupModel(getInstance().main_model);
}

void ModelContainer::setMainModel(std::string main_model) {
    if (!ModelContainer::lookupModel(main_model)) {
        Logging::error("Could not specify main model ", main_model,
                       ", because no such model is loaded");
        return;
    }
    Logging::info("Using ", main_model, " as primary model");
    getInstance().main_model = main_model;
}

const std::string &ModelContainer::getMainModel() {
    return getInstance().main_model;
}


ModelContainer &ModelContainer::getInstance() {
    static ModelContainer instance;
    return instance;
}

ModelContainer::~ModelContainer() {
// XXX uncomment the following, when fork has been replaced with threads
//    for (auto &entry : *this)  // pair<string, ConfigurationModel *>
//        delete entry.second;
}
