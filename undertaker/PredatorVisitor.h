/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#ifndef __PREDATOR_VISITOR_H__
#define __PREDATOR_VISITOR_H__

#include <Puma/PreTreeNodes.h>
#include <Puma/ErrorStream.h>

class PredatorVisitor : public Puma::PreVisitor {

    void iterateNodes (Puma::PreTree *);
    unsigned long _nodeNum;
    Puma::ErrorStream &_err;

public:
    PredatorVisitor(Puma::ErrorStream &err) : _nodeNum(0), _err(err) {};

    std::string buildExpression (Puma::PreTree *);
    void visitPreIfDirective_Pre (Puma::PreIfDirective *);
    void visitPreIfdefDirective_Pre (Puma::PreIfdefDirective *);
    void visitPreIfndefDirective_Pre (Puma::PreIfndefDirective *);
    void visitPreElifDirective_Pre (Puma::PreElifDirective *);
    void visitPreElseDirective_Pre (Puma::PreElseDirective *);
    void visitPreDefineConstantDirective_Pre (Puma::PreDefineConstantDirective *);
    void visitPreUndefDirective_Pre (Puma::PreUndefDirective *);
    void visitPreEndifDirective_Pre (Puma::PreEndifDirective *);
};

#endif
