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

#include <Puma/CTranslationUnit.h>
#include <Puma/Unit.h>
#include <Puma/ManipCommander.h>
#include <Puma/PreParser.h>
#include <Puma/Token.h>
#include <Puma/PreTreeNodes.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/PreSonIterator.h>

/// \brief cuts out all \#include statements
Puma::Unit *cut_includes(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e;

restart:
    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        switch(s->type()) {
        case TOK_PRE_ASSERT:
        case TOK_PRE_ERROR:
        case TOK_PRE_INCLUDE:
        case TOK_PRE_INCLUDE_NEXT:
        case TOK_PRE_WARNING:
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
    ss << "B" <<  _number;
    return ss.str();
}

ConditionalBlock *PumaConditionalBlock::parse(const char *filename,
                                              CppFile *cpp_file) {
    PumaConditionalBlockBuilder builder;
    return builder.parse(filename, cpp_file);
}

using namespace Puma;

// Build a string from a subtree of the preprocessor syntax tree.
char* buildString (const PreTree* node)
{
    assert(node);

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

    // eat trailing whitespace
    string::size_type i = str.length();
    while (str[i-1] == ' ' || str[i-1] == '\t')
        --i;
    str.erase(i);

    // Create the return string buffer.
    ptr = result = new char[str.size() + 5];

    // Copy the buffer into the return buffer but skip newlines.
    for (unsigned int i = 0; i < str.size(); i++)
        if (str[i] != '\n')
            *ptr++ = str[i];

    // Finish return buffer.
    *ptr = '\0';

    assert(result);
    return result;
}

std::string PumaConditionalBlock::ExpressionStr() const {
    assert(_parent);
    const PreTree *node;

    assert(_current_node);

   if (_expressionStr_cache)
      return std::string(_expressionStr_cache);

    if ((node = dynamic_cast<const PreIfDirective *>(_current_node)))
        _expressionStr_cache = buildString(node->son(1));
    else
    if ((node = dynamic_cast<const PreIfdefDirective *>(_current_node)))
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    else
    if ((node = dynamic_cast<const PreIfndefDirective *>(_current_node)))
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    else
    if ((node = dynamic_cast<const PreElifDirective *>(_current_node)))
        _expressionStr_cache = buildString(node->son(1));
    else
    if ((node = dynamic_cast<const PreElseDirective *>(_current_node)))
        _expressionStr_cache = (char *)"";

    if (_expressionStr_cache) {
        PreMacroExpander expander(_builder.cpp_parser());
        return expander.expandMacros(_expressionStr_cache);
    }
    else {
        return std::string("?? ") + typeid(node).name();
    }
}

bool PumaConditionalBlock::isIfndefine() const {
    if (dynamic_cast<const PreIfndefDirective *>(_current_node) != 0)
        return true;
    else
        return false;
}

void PumaConditionalBlockBuilder::iterateNodes (PreTree *node) {
    PreSonIterator i(node);

    for (i.first(); !i.isDone(); i.next())
        i.currentItem()->accept(*this);
}

ConditionalBlock *PumaConditionalBlockBuilder::parse(const char *filename, CppFile *cpp_file) {

    Puma::Unit *unit = _project.scanFile(filename);
    if (!unit) {
        std::cerr << "Failed to parse: " << filename << std::endl;
        return NULL;
    }

    unit = cut_includes(unit);
    Puma::CTranslationUnit *file = _parser.parse(*unit, _project, 2); // cpp tree only!

    Puma::PreTree *ptree = file->cpp_tree();
    if (!ptree) {
        std::cerr << "Failed to create cpp tree from file : " << filename << std::endl;
        return NULL;
    }

    _cpp = new PreprocessorParser(&_err, &_project.unitManager(), &file->local_units(), std::cerr);

    _file = cpp_file;
    _current = NULL;
    ptree->accept(*this);
    return _current;
}


PumaConditionalBlockBuilder::PumaConditionalBlockBuilder()
    : _cpp(0), _err(), _project(_err, 0, NULL), _parser() {}

PumaConditionalBlockBuilder::~PumaConditionalBlockBuilder() {
    delete _cpp;
}

#if 0
#define TRACECALL \
    std::cerr << __PRETTY_FUNCTION__ << " called " << std::endl
#else
#define TRACECALL
#endif

// @todo move this somwhere else
#ifndef __unused
#define __unused __attribute__((unused))
#endif

void PumaConditionalBlockBuilder::visitPreProgram_Pre (PreProgram *node) {
    TRACECALL;

    assert (!_current);
    _nodeNum = 0;
    _current = new PumaConditionalBlock(_file, NULL, NULL, node, 0, *this);
    _current->_isIfBlock = true;
    _current->_start = node->startToken();
    _current->_end   = node->endToken();
    _condBlockStack.push(_current);
}

void PumaConditionalBlockBuilder::visitPreProgram_Post (__unused PreProgram *node ) {
    TRACECALL;
    _condBlockStack.pop();
}

void PumaConditionalBlockBuilder::visitPreIfDirective_Pre (PreIfDirective *node) {
    TRACECALL;

    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, NULL, node, _nodeNum++, *this);
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
}

void PumaConditionalBlockBuilder::visitPreIfdefDirective_Pre (PreIfdefDirective *node) {
    TRACECALL;

    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, NULL, node, _nodeNum++, *this);
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
}

void PumaConditionalBlockBuilder::visitPreIfndefDirective_Pre (PreIfndefDirective *node) {
    TRACECALL;

    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, NULL, node, _nodeNum++, *this);
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
}

void PumaConditionalBlockBuilder::visitPreElifDirective_Pre (PreElifDirective *node) {
    TRACECALL;

    assert(_current);
    assert(_file);

    PumaConditionalBlock *prev = _condBlockStack.top();
    _condBlockStack.pop();
    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, prev, node, _nodeNum++, *this);
    _file->push_back(_current);
    _condBlockStack.push(_current);
}

void PumaConditionalBlockBuilder::visitPreElseDirective_Pre (PreElseDirective *node) {
    TRACECALL;

    assert(_current);
    assert(_file);

    PumaConditionalBlock *prev = _condBlockStack.top();
    _condBlockStack.pop();
    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, prev, node, _nodeNum++, *this);
    _file->push_back(_current);
    _condBlockStack.push(_current);

}

void PumaConditionalBlockBuilder::visitPreEndifDirective_Pre (__unused PreEndifDirective *node) {
    TRACECALL;

    _condBlockStack.pop();
    _current = _condBlockStack.top();
}

void PumaConditionalBlockBuilder::visitDefineHelper(PreTreeComposite *node, bool define) {
    const std::string definedFlag = node->son(1)->startToken()->text();
    PumaConditionalBlock &block = *_condBlockStack.top();
    
    CppFile::DefineMap &map = *_file->getDefines();
    CppFile::DefineMap::iterator i = map.find(definedFlag);
    if (i == map.end())
        // First define for this item
        map[definedFlag] = new CppDefine(&block, define, definedFlag);
    else
        (*i).second->newDefine(&block, define);

    block.addDefine(map[definedFlag]);
}

void PumaConditionalBlockBuilder::visitPreDefineConstantDirective_Pre (Puma::PreDefineConstantDirective *node){
    TRACECALL;
    visitDefineHelper(node, true);
}

void PumaConditionalBlockBuilder::visitPreUndefDirective_Pre (Puma::PreUndefDirective *node){
    TRACECALL;
    visitDefineHelper(node, false);
}
