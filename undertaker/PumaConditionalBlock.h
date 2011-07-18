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


#ifndef _PUMA_CONDITIONAL_BLOCK_H
#define _PUMA_CONDITIONAL_BLOCK_H

#include <Puma/PreTree.h>
#include <Puma/PreVisitor.h>
#include <Puma/PreprocessorParser.h>
#include <Puma/PreTreeNodes.h>
#include <Puma/CCParser.h>
#include <Puma/CParser.h>
#include <Puma/CProject.h>

#include <stack>

#include "StringJoiner.h"
#include "ConfigurationModel.h"
#include "ConditionalBlock.h"

// forward decl.
class PumaConditionalBlockBuilder;

class PumaConditionalBlock : public ConditionalBlock {
    static unsigned long numNodes;
    unsigned long _number;
    Puma::Token *_start, *_end;

    const Puma::PreTree *_current_node;
    
    bool _isIfBlock, _isIfndefine;
    PumaConditionalBlockBuilder &_builder;
    // For some reason, getting the expression string fails on
    // subsequent calls. We therefore cache the first result.
    mutable char *_expressionStr_cache;
    

public:
    PumaConditionalBlock(CppFile *file,
                         ConditionalBlock *parent,
                         ConditionalBlock *prev,
                         const Puma::PreTree *node,
                         PumaConditionalBlockBuilder &builder) :
        ConditionalBlock(file, parent, prev), _current_node(node),
        _isIfBlock(false), _isIfndefine(false), _builder(builder),
        _expressionStr_cache(NULL) {
        _number = numNodes++ -1;
        lateConstructor();
    };

    virtual ~PumaConditionalBlock() {}

    //! location related accessors
    virtual const char *filename()   const { return _start->location().filename().name(); };
    virtual unsigned int lineStart() const { return _start->location().line();   };
    virtual unsigned int colStart()  const { return _start->location().column(); };
    virtual unsigned int lineEnd()   const { return _end  ->location().line();   };
    virtual unsigned int colEnd()    const { return _end  ->location().column(); };
    /// @}

    //! \return original untouched expression
    virtual std::string ExpressionStr() const;
    virtual bool isIfBlock() const { return _isIfBlock; }
    virtual bool isIfndefine() const { return _isIfndefine; }
    virtual const std::string getName() const;


    static ConditionalBlock *parse(const char *filename, CppFile *cppfile);

    friend class PumaConditionalBlockBuilder;
};

class PumaConditionalBlockBuilder : public Puma::PreVisitor {

    long _depth;  // recursion depth
    void iterateNodes (Puma::PreTree *);
    // Stack of open conditional blocks. Pushed to when entering #ifdef
    // (and similar) blocks, popped from when leaving them.
    std::stack<PumaConditionalBlock*> _condBlockStack;
    PumaConditionalBlock* _current;
    CppFile *_file;
    Puma::PreprocessorParser *_cpp;
    Puma::ErrorStream _err;
    Puma::CProject _project;
    Puma::CParser _parser;

    void visitDefineHelper(Puma::PreTreeComposite *node, bool define);

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
    void visitPreUndefDirective_Pre (Puma::PreUndefDirective *);
};

#endif
