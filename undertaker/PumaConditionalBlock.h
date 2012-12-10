// -*- mode: c++ -*-
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


#ifndef _PUMA_CONDITIONAL_BLOCK_H
#define _PUMA_CONDITIONAL_BLOCK_H

#include <boost/shared_ptr.hpp>

#include <Puma/PreTree.h>
#include <Puma/PreVisitor.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/PreTreeNodes.h>
#include <Puma/CCParser.h>
#include <Puma/CParser.h>
#include <Puma/CProject.h>

#include <stack>
#include <list>
#include <ostream>

#include "StringJoiner.h"
#include "ConfigurationModel.h"
#include "ConditionalBlock.h"

// forward decl.
class PumaConditionalBlockBuilder;

class PumaConditionalBlock : public ConditionalBlock {
    unsigned long _number;
    Puma::Token *_start, *_end;

    const Puma::PreTree *_current_node;

    bool _isIfBlock;
    PumaConditionalBlockBuilder &_builder;
    // For some reason, getting the expression string fails on
    // subsequent calls. We therefore cache the first result.
    mutable char *_expressionStr_cache;

    boost::shared_ptr<Puma::CProject> _project;

public:
    PumaConditionalBlock(CppFile *file, ConditionalBlock *parent, ConditionalBlock *prev,
                         const Puma::PreTree *node, const unsigned long nodeNum,
                         PumaConditionalBlockBuilder &builder) :
        ConditionalBlock(file, parent, prev),
        _number(nodeNum), _start(0), _end(0), _current_node(node),
        _isIfBlock(false), _builder(builder),
            _expressionStr_cache(NULL) {
        lateConstructor();
    };

    virtual ~PumaConditionalBlock() {}

    //! location related accessors
    virtual const char *filename()   const { return cpp_file->getFilename().c_str(); };
    virtual unsigned int lineStart() const {
        return getParent() ? _start->location().line() : 0;
    };
    virtual unsigned int colStart()  const {
        return getParent() ? _start->location().column() : 0;
    };
    virtual unsigned int lineEnd()   const {
        return getParent() ? _end  ->location().line() : 0;
    };
    virtual unsigned int colEnd()    const {
        return getParent() ? _end  ->location().column() : 0;
    };
    /// @}

    Puma::Token *pumaStartToken() const { return _start; };
    Puma::Token *pumaEndToken() const { return _end; };
    Puma::Unit  *unit() const { return _current_node->startToken() ?
            _current_node->startToken()->unit() : 0; }

    //! \return original untouched expression
    virtual const char * ExpressionStr() const;
    virtual bool isIfBlock() const { return _isIfBlock; }
    virtual bool isIfndefine() const;
    virtual const std::string getName() const;

    static ConditionalBlock *parse(const char *filename, CppFile *cppfile);

    friend class PumaConditionalBlockBuilder;
};

class PumaConditionalBlockBuilder : public Puma::PreVisitor {

    unsigned long _nodeNum;
    void iterateNodes (Puma::PreTree *);
    // Stack of open conditional blocks. Pushed to when entering #ifdef
    // (and similar) blocks, popped from when leaving them.
    std::stack<PumaConditionalBlock*> _condBlockStack;
    PumaConditionalBlock* _current;
    CppFile *_file;
    Puma::PreprocessorParser *_cpp;
    std::ofstream null_stream;
    Puma::ErrorStream _err;
    Puma::CProject *_project;
    Puma::CParser _parser;
    Puma::Unit *_unit; // the unit we are working on

    static std::list<std::string> _includePaths;

    void visitDefineHelper(Puma::PreTreeComposite *node, bool define);
    Puma::Unit *resolve_includes(Puma::Unit *);
    void reset_MacroManager(Puma::Unit *unit);


public:
    PumaConditionalBlockBuilder();
    ~PumaConditionalBlockBuilder();
    ConditionalBlock *parse (const char *filename, CppFile *cpp_file);
    Puma::PreprocessorParser *cpp_parser() { return _cpp; }

    void visitPreProgram_Pre (Puma::PreProgram *);
    void visitPreProgram_Post (Puma::PreProgram *);
    void visitPreIfDirective_Pre (Puma::PreIfDirective *);
    void visitPreIfdefDirective_Pre (Puma::PreIfdefDirective *);
    void visitPreIfndefDirective_Pre (Puma::PreIfndefDirective *);
    void visitPreElifDirective_Pre (Puma::PreElifDirective *);
    void visitPreElseDirective_Pre (Puma::PreElseDirective *);
    void visitPreEndifDirective_Pre (Puma::PreEndifDirective *);
    void visitPreDefineConstantDirective_Pre (Puma::PreDefineConstantDirective *);
    void visitPreDefineFunctionDirective_Pre (Puma::PreDefineFunctionDirective *);
    void visitPreUndefDirective_Pre (Puma::PreUndefDirective *);

    static void addIncludePath(const char *);
};

#endif
