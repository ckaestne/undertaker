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

#include "PredatorVisitor.h"
#include <Puma/PreSonIterator.h>
#include <Puma/StrCol.h>
#include <Puma/CUnit.h>


void PredatorVisitor::iterateNodes (Puma::PreTree *node) {
    Puma::PreSonIterator i(node);

    for (i.first(); !i.isDone(); i.next())
        i.currentItem()->accept(*this);
}

std::string PredatorVisitor::buildExpression(Puma::PreTree *node) {
    std::stringstream ss;
    Puma::CUnit tmp(_err);

    for (Puma::Token *t = node->startToken(); t != node->endToken(); t = t->unit()->next(t))
        tmp << *t;
    tmp << Puma::endu;
    char *s = Puma::StrCol::buildString(&tmp);
    std::string ret(s);
    delete[] s;
    return ret;
}


void PredatorVisitor::visitPreIfDirective_Pre (Puma::PreIfDirective *node) {
    std::cout << buildExpression(node) << std::endl;
    std::cout << "B" << _nodeNum++ << std::endl;
}

void PredatorVisitor::visitPreIfdefDirective_Pre (Puma::PreIfdefDirective *node) {
    std::cout << buildExpression(node) << std::endl;
    std::cout << "B" << _nodeNum++ << std::endl;
}

void PredatorVisitor::visitPreIfndefDirective_Pre (Puma::PreIfndefDirective *node) {
    std::cout << buildExpression(node) << std::endl;
    std::cout << "B" << _nodeNum++ << std::endl;
}

void PredatorVisitor::visitPreElseDirective_Pre (Puma::PreElseDirective *node) {
//    std::cout << buildExpression(node) << std::endl; //XXX FIXE: why doesn't this work like it is?
    std::cout << "#else" << std::endl;
    std::cout << "B" << _nodeNum++ << std::endl;
}

void PredatorVisitor::visitPreElifDirective_Pre (Puma::PreElifDirective *node) {
    std::cout << buildExpression(node) << std::endl;
    std::cout << "B" << _nodeNum++ << std::endl;
}

void PredatorVisitor::visitPreDefineConstantDirective_Pre (Puma::PreDefineConstantDirective *node) {
    std::cout << buildExpression(node) << std::endl;
}

void PredatorVisitor::visitPreUndefDirective_Pre (Puma::PreUndefDirective *node) {
    std::cout << buildExpression(node) << std::endl;
}

void PredatorVisitor::visitPreEndifDirective_Pre (Puma::PreEndifDirective *node) {
    std::cout << buildExpression(node) << std::endl;
}

