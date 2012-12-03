// -*- mode: c++ -*-
/*
 * boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef KCONFIG_BOOLVISITOR_H
#define KCONFIG_BOOLVISITOR_H

#include "bool.h"

#include <map>

namespace kconfig {

    class BoolVisitor {

        friend class BoolExpVar;
        friend class BoolExpNot;
        friend class BoolExpConst;
        friend class BoolExpOr;
        friend class BoolExpAnd;
        friend class BoolExpImpl;
        friend class BoolExpEq;
        friend class BoolExp;
        friend class BoolExpCall;
        friend class BoolExpAny;

    public:
        bool isVisited(BoolExp *node) const {
            std::map<BoolExp *, void *>::const_iterator it;it = visited.find(node);
            return it != visited.end();
        }

    protected:
        std::map <BoolExp *, void *>visited;
        void *left, *right;
        void *result;
        virtual void visit(BoolExp *e) = 0;
        virtual void visit(BoolExpAnd *e) = 0;
        virtual void visit(BoolExpOr *e) = 0;
        virtual void visit(BoolExpNot *e) = 0;
        virtual void visit(BoolExpConst *e) = 0;
        virtual void visit(BoolExpVar *e) = 0;
        virtual void visit(BoolExpImpl *e) = 0;
        virtual void visit(BoolExpEq *e) = 0;
        virtual void visit(BoolExpCall *e) = 0;
        virtual void visit(BoolExpAny *e) = 0;
    };
}

#endif
