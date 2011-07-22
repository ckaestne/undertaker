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
#include <Puma/UnitManager.h>
#include <Puma/ManipCommander.h>
#include <Puma/PreParser.h>
#include <Puma/Token.h>
#include <Puma/TokenStream.h>
#include <Puma/PreTreeNodes.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/PreFileIncluder.h>
#include <Puma/PreSonIterator.h>

#include <set>

/// \brief cuts out all bad CPP statements
Puma::Unit * remove_cpp_statements(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e;

restart:
    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        switch(s->type()) {
        case TOK_PRE_ASSERT:
        case TOK_PRE_ERROR:
            //        case TOK_PRE_INCLUDE:
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
    else if ((node = dynamic_cast<const PreIfdefDirective *>(_current_node)))
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    else if ((node = dynamic_cast<const PreIfndefDirective *>(_current_node))) {
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    } else if ((node = dynamic_cast<const PreElifDirective *>(_current_node)))
        _expressionStr_cache = buildString(node->son(1));
    else if ((node = dynamic_cast<const PreElseDirective *>(_current_node)))
        _expressionStr_cache = (char *)"";


    if (_expressionStr_cache) {
        PreMacroExpander expander(_builder.cpp_parser());
        _expressionStr_cache = expander.expandMacros(_expressionStr_cache);
        return _expressionStr_cache;
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

    unit = remove_cpp_statements(unit);

    CTranslationUnit *tu = new CTranslationUnit (*unit, _project);

    // prepare C preprocessor
    TokenStream stream;           // linearize tokens from several files
    stream.push (unit);
    _project.unitManager ().init ();

    _cpp = new PreprocessorParser(&_err, &_project.unitManager(), &tu->local_units(), std::cerr);
    _cpp->macroManager ()->init (unit->name ());
    _cpp->stream (&stream);
    _cpp->configure (_project.config ());

    /* Resolve all #include statements, must be done after _cpp
       initialization */
    unit = resolve_includes(unit);
    stream.reset();
    stream.push(unit);

    _cpp->silentMode ();
    _cpp->parse ();
    /* After parsing we have to reset the macro manager */
    reset_MacroManager(unit);

    Puma::PreTree *ptree = _cpp->syntaxTree();
    if (!ptree) {
        std::cerr << "Failed to create cpp tree from file : " << filename << std::endl;
        return NULL;
    }

    _file = cpp_file;
    _current = NULL;
    ptree->accept(*this);
    return _current;
}


PumaConditionalBlockBuilder::PumaConditionalBlockBuilder()
    : _cpp(0), null_stream("/dev/null"), _err(null_stream), _project(_err, 0, NULL), _parser() { }

PumaConditionalBlockBuilder::~PumaConditionalBlockBuilder() {
    delete _cpp;
}

#if 0
#define TRACECALL \
    std::cerr << __PRETTY_FUNCTION__ << ": " \
              << "Start: " << node->startToken()->location().line() << ", "       \
              << "End: " << node->endToken()->location().line() \
              << std::endl
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
    _current->_start = node->startToken();
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
    parent->push_back(_current);
}

void PumaConditionalBlockBuilder::visitPreIfdefDirective_Pre (PreIfdefDirective *node) {
    TRACECALL;

    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, NULL, node, _nodeNum++, *this);
    _current->_start = node->startToken();
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
    parent->push_back(_current);
}

void PumaConditionalBlockBuilder::visitPreIfndefDirective_Pre (PreIfndefDirective *node) {
    TRACECALL;

    PumaConditionalBlock *parent = _condBlockStack.top();
    _current = new PumaConditionalBlock(_file, parent, NULL, node, _nodeNum++, *this);
    _current->_start = node->startToken();
    _condBlockStack.push(_current);
    _current->_isIfBlock = true;
    _file->push_back(_condBlockStack.top());
    parent->push_back(_current);
}

void PumaConditionalBlockBuilder::visitPreElifDirective_Pre (PreElifDirective *node) {
    TRACECALL;

    assert(_current);
    assert(_file);

    PumaConditionalBlock *prev = _condBlockStack.top();
    _condBlockStack.pop();
    PumaConditionalBlock *parent = _condBlockStack.top();
    _current->_end = node->startToken();
    _current = new PumaConditionalBlock(_file, parent, prev, node, _nodeNum++, *this);
    _current->_start = node->startToken();
    _file->push_back(_current);
    _condBlockStack.push(_current);
    parent->push_back(_current);
}

void PumaConditionalBlockBuilder::visitPreElseDirective_Pre (PreElseDirective *node) {
    TRACECALL;

    assert(_current);
    assert(_file);

    PumaConditionalBlock *prev = _condBlockStack.top();
    _condBlockStack.pop();
    PumaConditionalBlock *parent = _condBlockStack.top();
    _current->_end = node->startToken();
    _current = new PumaConditionalBlock(_file, parent, prev, node, _nodeNum++, *this);
    _current->_start = node->startToken();
    _file->push_back(_current);
    _condBlockStack.push(_current);
    parent->push_back(_current);
}

void PumaConditionalBlockBuilder::visitPreEndifDirective_Pre (__unused PreEndifDirective *node) {
    TRACECALL;

    _condBlockStack.pop();
    _current->_end = node->startToken();
    _current = _condBlockStack.top();
}

void PumaConditionalBlockBuilder::visitDefineHelper(PreTreeComposite *node, bool define) {
    const std::string definedFlag = node->son(1)->startToken()->text();
    const Puma::DString &definedDFlag = node->son(1)->startToken()->dtext();

    /* Don't handle function macros */
    if (cpp_parser()->macroManager()->getMacro(definedDFlag) != 0)
        return;

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

    const Puma::DString &definedFlag = node->son(1)->startToken()->dtext();
    cpp_parser()->macroManager()->removeMacro(definedFlag);
}

void PumaConditionalBlockBuilder::visitPreDefineFunctionDirective_Pre (Puma::PreDefineFunctionDirective * node){
    const Puma::DString &definedFlag = node->son(1)->startToken()->dtext();

    if (!_current->getParent()) { // Handle only toplevel defines
        if (node->sons() == 6) { // With parameter list
            char *expansion = buildString(node->son(5));

            Puma::PreMacro * macro = new PreMacro(node->son(1)->startToken()->dtext(),
                                                  node->son(3), expansion);
            delete[] expansion;
            cpp_parser()->macroManager ()->addMacro (macro);

        } else if (node->sons() == 5) { // Without parameter list
            char *expansion = buildString(node->son(4));

            Puma::PreMacro * macro = new PreMacro(node->son(1)->startToken()->dtext(),
                                                  (PreTree *) 0, expansion);
            delete[] expansion;
            cpp_parser()->macroManager ()->addMacro (macro);
        }
    } else {
        /* If an macro is defined in an block we can't expand them for
           sure anymore TODO Evaluate*/
        cpp_parser()->macroManager()->removeMacro(definedFlag);
    }
}

std::list<std::string> PumaConditionalBlockBuilder::_includePaths;

void PumaConditionalBlockBuilder::addIncludePath(const char *path){
    _includePaths.push_back(path);
}

Puma::Unit * PumaConditionalBlockBuilder::resolve_includes(Puma::Unit *unit) {
    Puma::PreFileIncluder includer(*_cpp);
    Puma::ManipCommander mc;
    Puma::Token *s, *e;
    std::string include;
    std::set<Puma::Unit *> already_seen;

    for (std::list<std::string>::const_iterator i = _includePaths.begin();
         i != _includePaths.end(); ++i) {
        includer.addIncludePath((*i).c_str());
    }

restart:
    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        if (s->type() == TOK_PRE_INCLUDE) {
            e = s;
            include.clear();
            do {
                e = unit->next(e);
                include += e->text();
            } while (unit->next(e) && unit->next(e)->text()[0] != '\n');

            Puma::Unit *file = includer.includeFile(include.c_str());
            if (file && already_seen.count(file) == 0) {
                /* Paste the included file only, if we haven't it seen
                   until then */
                mc.paste_before(s, file);
                already_seen.insert(file);
            }
            mc.kill(s, e);
            mc.commit();
            goto restart;
        }
    }

    return unit;
}

void PumaConditionalBlockBuilder::reset_MacroManager(Puma::Unit *unit) {
    Puma::Token *s, *e;

    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        if (s->type() == TOK_PRE_DEFINE) {
            e = s;
            do {
                e = unit->next(e);
            } while (e->is_whitespace());
            cpp_parser()->macroManager()->removeMacro(e->dtext());
        }
    }
}
