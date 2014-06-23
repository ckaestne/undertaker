// -*- mode: c++ -*-
/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#ifndef KCONFIG_SYMBOLTOOLS_H
#define KCONFIG_SYMBOLTOOLS_H

#ifndef LKC_DIRECT_LINK
#define LKC_DIRECT_LINK
#endif
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "lkc.h"
#pragma GCC diagnostic warning "-Wunused-parameter"

namespace kconfig {
    bool hasPrompt(struct symbol *sym);

    expr * visibilityExpression(struct symbol *sym);
    expr * dependsExpression(struct symbol *sym);
    /* this function is ONLY valid for bool/tristate defaults! */
    expr * defaultExpression_bool_tristate(struct symbol *sym);
    expr * reverseDepExpression(struct symbol *sym);
    expr * choiceExpression(struct symbol *sym);
    expr * selectExpression(struct symbol *sym);

    void nameSymbol(struct symbol *sym);
    struct symbol *getModulesSym(void);
}
#endif
