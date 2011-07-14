/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


#include "PumaConditionalBlock.h"
#include "StringJoiner.h"

#include <Puma/CCParser.h>
#include <Puma/CParser.h>
#include <Puma/CProject.h>
#include <Puma/CTranslationUnit.h>
#include <Puma/Unit.h>
#include <Puma/ManipCommander.h>
#include <Puma/PreParser.h>
#include <Puma/Token.h>
#include <Puma/PreTreeNodes.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/PreSonIterator.h>

unsigned long PumaConditionalBlock::numNodes;

/// \brief cuts out all \#include statements
Puma::Unit *cut_includes(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e;

restart:
    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        if (s->type() == TOK_PRE_INCLUDE) {
            e = s;
            do {
                e = unit->next(e);
            } while (e->text()[0] != '\n');
            mc.kill(s, e);
            mc.commit();
            goto restart;
        }
    }
    return unit;
}

const std::string PumaConditionalBlock::getName() const {
    std::stringstream ss;
    if (!_parent) return "B00"; // top level block, represents file
    ss << "B" + _number;
    return ss.str();
}

ConditionalBlock *PumaConditionalBlock::parse(const char *filename,
                                              CppFile *cpp_file) {
    Puma::ErrorStream err;
    Puma::CProject project(err, 0, NULL);
    Puma::CParser parser;
    PumaConditionalBlockBuilder builder;

    Puma::Unit *unit = project.scanFile(filename);
    if (!unit) {
        std::cerr << "Failed to parse: " << filename << std::endl;
        return NULL;
    }

    unit = cut_includes(unit);
    Puma::CTranslationUnit *file = parser.parse(*unit, project, 2); // cpp tree only!

    Puma::PreTree *ptree = file->cpp_tree();
    if (!ptree) {
        std::cerr << "Failed to create cpp tree from file : " << filename << std::endl;
        return NULL;
    }

    return builder.parse(cpp_file, ptree);
}

using namespace Puma;

void PumaConditionalBlockBuilder::iterateNodes (PreTree *node) {
    PreSonIterator i(node);

    for (i.first(); !i.isDone(); i.next())
        i.currentItem()->accept(*this);
}

ConditionalBlock *PumaConditionalBlockBuilder::parse(CppFile *file, Puma::PreTree *tree) {
    _file = file;
    tree->accept(*this);
    return _parent;
}
// Build a string from a subtree of the preprocessor syntax tree.
char* buildString (const PreTree* node)
{
    if (! node) return (char*) 0;

    char *result, *ptr;
    string str;

    // If subtree isn't empty concatenate all tokens to a single string.
    if (node->sons ()) {
        PreTokenListPart* list = (PreTokenListPart*) node->son (0);

        // Fill the buffer.
        for (unsigned int i = 0; i < (unsigned int) list->sons (); i++) {
            str += ((PreTreeToken*) list->son (i))->token ()->text ();
        }
    }

    // Create the return string buffer.
    ptr = result = new char[str.size() + 5];

    // Copy the buffer into the return buffer but skip newlines.
    for (unsigned int i = 0; i < str.size(); i++)
        if (str[i] != '\n')
            *ptr++ = str[i];

    // Finish return buffer.
    *ptr = '\0';

    return result;
}

void PumaConditionalBlockBuilder::visitPreProgram_Pre (PreProgram *node) {
    assert (!_parent);
    _prev = 0;
    _parent = new PumaConditionalBlock(_file, NULL, NULL);
    _parent->_isIfBlock = true;
    _parent->_start = node->startToken();
    _parent->_end   = node->endToken();
}

void PumaConditionalBlockBuilder::visitPreConditionalGroup_Pre (PreConditionalGroup *node) {
    _prev   = NULL;
    _parent = new PumaConditionalBlock(_file, _parent, _prev);
    _parent->_start = node->startToken();
    _parent->_end = node->endToken();
}

void PumaConditionalBlockBuilder::visitPreConditionalGroup_Post (PreConditionalGroup *node) {
    _prev   = NULL;
    _parent->_end = node->endToken();
    _parent = (PumaConditionalBlock *)_parent->getParent();
}

void PumaConditionalBlockBuilder::visitPreIfDirective_Pre (PreIfDirective *node) {
    _prev->_isIfBlock = true;
    _prev->_expr = std::string(buildString(node->son(1)));
    _parent->_end = node->startToken()->unit()->prev(node->startToken());
}

void PumaConditionalBlockBuilder::visitPreIfdefDirective_Pre (PreIfdefDirective *node) {
    _prev->_isIfBlock = true;
    _prev->_expr = std::string(buildString(node->son(1)));
}

void PumaConditionalBlockBuilder::visitPreIfndefDirective_Pre (PreIfndefDirective *node) {
    _prev->_isIfBlock = true;
    _prev->_isIfndefine = true;
    _prev->_expr = std::string(buildString(node->son(1)));
    _parent->_end = node->startToken()->unit()->prev(node->startToken());
}

void PumaConditionalBlockBuilder::visitPreElifDirective_Pre (PreElifDirective *node) {
    _prev->_end = node->startToken()->unit()->prev(node->startToken());    
    _prev = new PumaConditionalBlock(_file, _parent, _prev);
}

void PumaConditionalBlockBuilder::visitPreElseDirective_Pre (PreElseDirective *node) {
    _prev->_end = node->startToken()->unit()->prev(node->startToken());    
    _prev = new PumaConditionalBlock(_file, _parent, _prev);
}
