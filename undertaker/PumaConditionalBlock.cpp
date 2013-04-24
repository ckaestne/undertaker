/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
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
#include "Logging.h"

#include <Puma/CTranslationUnit.h>
#include <Puma/CUnit.h>
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

static inline Puma::Token *
puma_token_next_newline(Puma::Token *e, Puma::Unit *unit) {
    do {
        e = unit->next(e);
    } while (e && e->text()[0] != '\n' && !strchr(e->text(), '\n'));
    return e;
}

/// \brief cuts out all bad CPP statements
Puma::Unit *remove_cpp_statements(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e, *prev;

    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        switch(s->type()) {
        case TOK_PRE_ASSERT:
        case TOK_PRE_ERROR:
            //        case TOK_PRE_INCLUDE:
        case TOK_PRE_INCLUDE_NEXT:
        case TOK_PRE_WARNING:
            prev = unit->prev(s);
            e = puma_token_next_newline(s, unit);
            mc.kill(s, e);
            mc.commit();
            s = prev ? prev : unit->first();
        }
    }
    return unit;
}

/// \brief replaces #define CONFIG_FOO 0 -> #undef CONFIG_FOO
Puma::Unit *normalize_define_null(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e, *prev;

    for (s = unit->first(); s != unit->last(); s=unit->next(s)) {
        if (!s->is_preprocessor())
            continue;

        if (s->type() == TOK_PRE_DEFINE) {
            Puma::ErrorStream err;
            Puma::CUnit undef(err);
            // I have no idea why I need to skip a token here
            Puma::Token *ident = unit->next(unit->next(s));
            Puma::Token *what = unit->next(unit->next(ident));

            if (ident->type() == Puma::TOK_ID && !strcmp(what->text(), "0")) {
//                logger << error << "Token text is " << what->text() << std::endl;
                prev = unit->prev(s);
                e = puma_token_next_newline(s, unit);
                undef << "#undef " << *ident
                      << std::endl << Puma::endu;
                mc.replace(s, e, undef.first(), undef.last());
                mc.commit();
                s = prev ? prev : unit->first();
//                logger << error << undef << std::endl;
            }
        }
    }
    return unit;
}

const std::string PumaConditionalBlock::getName() const {
    std::stringstream ss;
    if (!_parent)
        return "B00"; // top level block, represents file
    else {
        ss << "B" <<  _number;
        if (useBlockWithFilename)
            ss << "_" << this->normalize_filename(this->filename());
        return ss.str();
    }
}

ConditionalBlock *PumaConditionalBlock::parse(const char *filename,
                                              CppFile *cpp_file) {
    PumaConditionalBlockBuilder builder;
    return builder.parse(filename, cpp_file);
}

using namespace Puma;

// Build a string from a subtree of the preprocessor syntax tree.
char* buildString (const PreTree* node) {
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

const char * PumaConditionalBlock::ExpressionStr() const {
    assert(_parent);
    const PreTree *node;

    assert(_current_node);

    if (_expressionStr_cache)
      return _expressionStr_cache;

    if ((node = dynamic_cast<const PreIfDirective *>(_current_node)))
        _expressionStr_cache = buildString(node->son(1));
    else if ((node = dynamic_cast<const PreIfdefDirective *>(_current_node)))
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    else if ((node = dynamic_cast<const PreIfndefDirective *>(_current_node)))
        _expressionStr_cache = strdup(node->son(1)->startToken()->text());
    else if ((node = dynamic_cast<const PreElifDirective *>(_current_node)))
        _expressionStr_cache = buildString(node->son(1));
    else if (isElseBlock()) {
        _expressionStr_cache = (char *)"";
        return _expressionStr_cache;
    }

    if (_expressionStr_cache) {
        PreMacroExpander expander(_builder.cpp_parser());
        _expressionStr_cache = expander.expandMacros(_expressionStr_cache);
        return _expressionStr_cache;
    } else {
        return "??";
    }
}

bool PumaConditionalBlock::isIfndefine() const {
    if (dynamic_cast<const PreIfndefDirective *>(_current_node) != 0)
        return true;
    else
        return false;
}

bool PumaConditionalBlock::isElseIfBlock() const {
    if (dynamic_cast<const PreElifDirective *>(_current_node) != 0)
        return true;
    else
        return false;
}

bool PumaConditionalBlock::isElseBlock() const {
    if (dynamic_cast<const PreElseDirective *>(_current_node) != 0)
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
    Puma::Unit *unit = _project->scanFile(filename);
    if (!unit) {
        logger << error << "Failed to parse: " << filename << std::endl;
        return NULL;
    }

    _unit = normalize_define_null(remove_cpp_statements(unit));

    CTranslationUnit *tu = new CTranslationUnit (*_unit, *_project);

    // prepare C preprocessor
    TokenStream stream;           // linearize tokens from several files
    stream.push (_unit);
    _project->unitManager ().init ();

    _cpp = new PreprocessorParser(&_err, &_project->unitManager(), &tu->local_units(), std::cerr);
    _cpp->macroManager ()->init (unit->name ());
    _cpp->stream (&stream);
    _cpp->configure (_project->config ());

    /* Resolve all #include statements, must be done after _cpp
       initialization */
    _unit = resolve_includes(_unit);
    stream.reset();
    stream.push(unit);

    _cpp->silentMode ();
    _cpp->parse ();
    /* After parsing we have to reset the macro manager */
    reset_MacroManager(_unit);

    Puma::PreTree *ptree = _cpp->syntaxTree();
    if (!ptree) {
        logger << error << "Failed to create cpp tree from file : " << filename << std::endl;
        return NULL;
    }

    _file = cpp_file;
    _current = NULL;
    ptree->accept(*this);
    /* Copy the Puma::CProject to the shared_ptr instances of all
       blocks within the file */
    boost::shared_ptr<Puma::CProject> sh_project(_project);
    for (CppFile::iterator it = _file->begin(); it != _file->end(); ++it) {
        ((PumaConditionalBlock *)(*it))->_project = sh_project ;
    }
    return _current;
}


PumaConditionalBlockBuilder::PumaConditionalBlockBuilder()
    : _cpp(0), null_stream("/dev/null"), _err(null_stream), _parser() {
    _project = new Puma::CProject(_err, 0, NULL);
}

PumaConditionalBlockBuilder::~PumaConditionalBlockBuilder() {
    delete _cpp;
}

#if 0
#define TRACECALL \
    logger << error << __PRETTY_FUNCTION__ << ": "                   \
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
    if (node->startToken()) {
        TRACECALL;
    }

    assert (!_current);
    assert (_unit);

    _nodeNum = 0;
    _current = new PumaConditionalBlock(_file, NULL, NULL, node, 0, *this);
    _current->_isIfBlock = true;
    _current->_start = node->startToken();
    _current->_end   = node->endToken();
    _condBlockStack.push(_current);
}

void PumaConditionalBlockBuilder::visitPreProgram_Post (__unused PreProgram *node ) {
    if (node->startToken()) {
        TRACECALL;
    }
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

// Remove an possible include guard, this was copied from libPuma::PreFileIncluder
static Puma::Unit * removeIncludeGuard(Unit *unit) {
    Puma::ManipCommander mc;

    Token *guard = 0, *ifndef, *end_define, *endif;
    Token *tok = (Token*)unit->first ();
    // skip comments and whitespace
    while (tok && (tok->is_whitespace () || tok->is_comment ()))
        tok = (Token*)unit->next (tok);
    // the next token has to be #ifndef
    if (!(tok && tok->is_preprocessor () && tok->type () == TOK_PRE_IFNDEF))
        return unit;
    ifndef = tok;
    tok = (Token*)unit->next (tok);
    // now whitespace
    if (!(tok && tok->is_whitespace ()))
        return unit;
    tok = (Token*)unit->next (tok);
    // the next has be an identifier => the name of the guard macro
    if (!(tok && tok->is_identifier ()))
        return unit;
    guard = tok;
    tok = (Token*)unit->next (tok);
    // skip comments and whitespace
    while (tok && (tok->is_whitespace () || tok->is_comment ()))
        tok = (Token*)unit->next (tok);
    // the next token has to be #define
    if (!(tok && tok->is_preprocessor () && tok->type () == TOK_PRE_DEFINE))
        return unit;
    tok = (Token*)unit->next (tok);
    // now whitespace
    if (!(tok && tok->is_whitespace ()))
        return unit;
    tok = (Token*)unit->next (tok);
    // the next has be an identifier => the name of the guard macro
    if (!(tok && tok->is_identifier ()))
        return unit;
    // check if the identifier is our guard variable
    if (strcmp (tok->text (), guard->text ()) != 0)
        return unit;
    tok = (Token*)unit->next (tok);
    end_define = tok;
    // find the corresponding #endif
    int level = 1;
    while (tok) {
        if (tok->is_preprocessor ()) {
            if (tok->type () == TOK_PRE_IF || tok->type () == TOK_PRE_IFDEF ||
                tok->type () == TOK_PRE_IFNDEF)
                level++;
            else if (tok->type () == TOK_PRE_ENDIF) {
                endif = tok;
                level--;
                if (level == 0)
                    break;
            }
        }
        tok = (Token*)unit->next (tok);
    }
    if (level > 0)
        return unit;
    tok = (Token*)unit->next (tok);
    // skip comments and whitespace
    while (tok && (tok->is_whitespace () || tok->is_comment ()))
        tok = (Token*)unit->next (tok);
    // here we should have reached the end of the unit!
    if (tok)
        return unit;

    mc.kill(ifndef, end_define);
    mc.kill(endif, unit->last());
    mc.commit();
    return unit;
}

Puma::Unit *PumaConditionalBlockBuilder::resolve_includes(Puma::Unit *unit) {
    Puma::PreFileIncluder includer(*_cpp);
    Puma::ManipCommander mc;
    Puma::Token *s, *e;
    std::string include;
    std::set<Puma::Unit *> already_seen;

    for (std::list<std::string>::const_iterator i = _includePaths.begin();
         i != _includePaths.end(); ++i) {
        includer.addIncludePath((*i).c_str());
    }

    for (s = unit->first(); s != unit->last() && s; s = unit->next(s)) {
        if (s->type() == TOK_PRE_INCLUDE) {
            e = s;
            include.clear();
            do {
                e = unit->next(e);
                include += e->text();
            } while (unit->next(e) && unit->next(e)->text()[0] != '\n');

            Puma::Unit *file = includer.includeFile(include.c_str());
            Puma::Token *before = unit->prev(s);
            if (file && already_seen.count(file) == 0) {
                /* Paste the included file only, if we haven't it seen
                   until then */
                file = removeIncludeGuard(file);
                mc.paste_before(s, file);
                already_seen.insert(file);
            }
            mc.kill(s, e);
            mc.commit();
            /* Jump before the included file */
            s = before ? before : unit->first();
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
