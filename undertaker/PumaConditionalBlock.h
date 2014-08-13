// -*- mode: c++ -*-
/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Bernhard Heinloth <bernhard@heinloth.net>
 * Copyright (C) 2012 Valentin Rothberg <valentinrothberg@gmail.com>
 * Copyright (C) 2012 Andreas Ruprecht  <rupran@einserver.de>
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


#ifndef _PUMA_CONDITIONAL_BLOCK_H
#define _PUMA_CONDITIONAL_BLOCK_H

#include "ConditionalBlock.h"

// unique_ptr needs a complete type
#include <Puma/CTranslationUnit.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/CProject.h>
// inheritance for PumaConditionalBlockBuilder
#include <Puma/PreVisitor.h>
// Visitor node types
#include <Puma/PreTreeNodes.h>

#include <stack>
#include <list>
#include <fstream>

// forward decl.
class PumaConditionalBlockBuilder;
namespace Puma {
    class PreTree;
}


/************************************************************************/
/* PumaConditionalBlock                                                 */
/************************************************************************/

class PumaConditionalBlock : public ConditionalBlock {
    unsigned long _number;
    Puma::Token *_start = nullptr, *_end = nullptr;

    const Puma::PreTree *_current_node;

    bool _isIfBlock = false;
    bool _isDummyBlock = false;
    PumaConditionalBlockBuilder &_builder;
    // For some reason, getting the expression string fails on
    // subsequent calls. We therefore cache the first result.
    mutable char *_expressionStr_cache = nullptr;

public:
    PumaConditionalBlock(CppFile *file, ConditionalBlock *parent, ConditionalBlock *prev,
            const Puma::PreTree *node, const unsigned long nodeNum,
            PumaConditionalBlockBuilder &builder) :
            ConditionalBlock(file, parent, prev), _number(nodeNum),
            _current_node(node), _builder(builder) {
        lateConstructor();
    };

    virtual ~PumaConditionalBlock() { delete[] _expressionStr_cache; }

    //! location related accessors
    virtual unsigned int lineStart()     const final override {
        return getParent() ? _start->location().line() : 0;
    };
    virtual unsigned int colStart()      const final override {
        return getParent() ? _start->location().column() : 0;
    };
    virtual unsigned int lineEnd()       const final override {
        return getParent() ? _end  ->location().line() : 0;
    };
    virtual unsigned int colEnd()        const final override {
        return getParent() ? _end  ->location().column() : 0;
    };
    /// @}

    Puma::Token *pumaStartToken() const { return _start; };
    Puma::Token *pumaEndToken() const { return _end; };
    Puma::Unit  *unit() const {
        return _current_node->startToken() ? _current_node->startToken()->unit() : nullptr;
    }

    //! \return original untouched expression
    virtual const char * ExpressionStr() const final override;
    virtual bool isIfBlock()             const final override { return _isIfBlock; }
    virtual bool isIfndefine()           const final override;
    virtual bool isElseIfBlock()         const final override;
    virtual bool isElseBlock()           const final override;
    virtual bool isDummyBlock()          const final override { return _isDummyBlock; }
    virtual void setDummyBlock()               final override { _isDummyBlock = true; }
    virtual const std::string getName()  const final override;
    PumaConditionalBlockBuilder &getBuilder() const { return _builder; }

    friend class PumaConditionalBlockBuilder;
};


/************************************************************************/
/* PumaConditionalBlockBuilder                                          */
/************************************************************************/

class PumaConditionalBlockBuilder : public Puma::PreVisitor {
    unsigned long _nodeNum;
    virtual void iterateNodes (Puma::PreTree *) final override;
    // Stack of open conditional blocks. Pushed to when entering #ifdef
    // (and similar) blocks, popped from when leaving them.
    std::stack<PumaConditionalBlock *> _condBlockStack;
    PumaConditionalBlock* _current = nullptr;
    ConditionalBlock *_top = nullptr;
    CppFile *_file;
    std::ofstream null_stream;
    Puma::ErrorStream _err;
    // order seems to be important here, do not exchange!
    std::unique_ptr<Puma::CProject> _project;
    std::unique_ptr<Puma::CTranslationUnit> _tu;
    std::unique_ptr<Puma::PreprocessorParser> _cpp;

    Puma::Unit *_unit; // the unit we are working on

    static std::list<std::string> _includePaths;

    void visitDefineHelper(Puma::PreTreeComposite *node, bool define);
    void resolve_includes(Puma::Unit *);
    void reset_MacroManager(Puma::Unit *unit);
    ConditionalBlock *parse(const std::string &filename);

public:
    PumaConditionalBlockBuilder(CppFile *file, const std::string &filename) : _file(file),
            null_stream("/dev/null"), _err(null_stream) {
        _top = parse(filename);
    }

    ~PumaConditionalBlockBuilder() { _cpp->freeSyntaxTree(); };
    Puma::PreprocessorParser *cpp_parser() { return _cpp.get(); }

    ConditionalBlock *topBlock() { return _top; }

    virtual void visitPreProgram_Pre (Puma::PreProgram *)                 final override;
    virtual void visitPreProgram_Post (Puma::PreProgram *)                final override;
    virtual void visitPreIfDirective_Pre (Puma::PreIfDirective *)         final override;
    virtual void visitPreIfdefDirective_Pre (Puma::PreIfdefDirective *)   final override;
    virtual void visitPreIfndefDirective_Pre (Puma::PreIfndefDirective *) final override;
    virtual void visitPreElifDirective_Pre (Puma::PreElifDirective *)     final override;
    virtual void visitPreElseDirective_Pre (Puma::PreElseDirective *)     final override;
    virtual void visitPreEndifDirective_Pre (Puma::PreEndifDirective *)   final override;
    virtual void visitPreDefineConstantDirective_Pre (Puma::PreDefineConstantDirective *)
                                                                          final override;
    virtual void visitPreDefineFunctionDirective_Pre (Puma::PreDefineFunctionDirective *)
                                                                          final override;
    virtual void visitPreUndefDirective_Pre (Puma::PreUndefDirective *)   final override;

    unsigned long * getNodeNum() { return &_nodeNum; }
    static void addIncludePath(const char *);
};
#endif
