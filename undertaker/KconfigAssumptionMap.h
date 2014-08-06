// -*- mode: c++ -*-
/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KCONFIG_ASSUMPTIONMAP_H
#define KCONFIG_ASSUMPTIONMAP_H

#include <map>
#include <string>
#include <istream>


namespace kconfig {
    class PicosatCNF;

    /**
     * Read partial configuration fragments
     *
     * This represents a partial configuration (sometimes called a
     * configuration fragement).
     */
    class KconfigAssumptionMap : public std::map<std::string, bool> {
        PicosatCNF *_model;
    public:
        //! loads the given models
        KconfigAssumptionMap(PicosatCNF *model) : _model(model) { };

        /**
         * \brief load a partial configuration
         *
         * \return the number of items that have been processed
         */
        size_type readAssumptionsFromFile(std::istream &i);
    };
}
#endif
