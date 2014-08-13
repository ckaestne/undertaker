/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

// -*- mode: c++ -*-
#ifndef modelcontainer_h__
#define modelcontainer_h__

#include <string>
#include <map>

class ConfigurationModel;


/**
 * \brief Container that maps ConfigurationModel classes to its architectures
 *
 * This class is basically a singleton that derives from
 * std::map<std::string, ConfigurationModel*>. It provides a few
 * convenience methods for model loading and lookups.
 */
class ModelContainer : public std::map<std::string, ConfigurationModel*> {
    ModelContainer() = default;
    ~ModelContainer();

    std::string main_model;

public:
    ///< load models from the given directory
    static ConfigurationModel *loadModels(std::string modeldir);
    static ConfigurationModel *lookupModel(const std::string &arch);
    static const std::string lookupArch(const ConfigurationModel *model);
    static ModelContainer &getInstance();

    static ConfigurationModel *lookupMainModel();
    static void setMainModel(std::string);

    /// returns the main model as string or nullptr, if not set
    static const std::string &getMainModel();
};

#endif
