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

#include "StringJoiner.h"
#include "ConfigurationModel.h"
#include "ConditionalBlock.h"

class PumaConditionalBlock : public ConditionalBlock {
    static unsigned long numNodes;
    unsigned long _number;
    Puma::Token *_start, *_end;

    std::string _expr;
    bool _isIfBlock, _isIfndefine;
    

public:
    PumaConditionalBlock(CppFile *file,
                         ConditionalBlock *parent,
                         ConditionalBlock *prev) :
        ConditionalBlock(file, parent, prev),
        _isIfBlock(false), _isIfndefine(false) {
        lateConstructor();
        _number = numNodes++;
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
    virtual std::string ExpressionStr() const { return _expr; }
    virtual bool isIfBlock() const { return _isIfBlock; }
    virtual bool isIfndefine() const { return _isIfndefine; }
    virtual const std::string getName() const;


    static ConditionalBlock *parse(const char *filename, CppFile *cppfile);

    friend class PumaConditionalBlockBuilder;
};

class PumaConditionalBlockBuilder : public Puma::PreVisitor {
protected:
    long _depth;  // recursion depth
    void iterateNodes (Puma::PreTree *);
    PumaConditionalBlock *_parent, *_prev;
    CppFile *_file;
public:
    ConditionalBlock *parse (CppFile *file, Puma::PreTree *tree);

    void visitPreProgram_Pre (Puma::PreProgram *);
    void visitPreConditionalGroup_Pre (Puma::PreConditionalGroup *);
    void visitPreConditionalGroup_Post (Puma::PreConditionalGroup *);
    void visitPreIfDirective_Pre (Puma::PreIfDirective *);
    void visitPreIfdefDirective_Pre (Puma::PreIfdefDirective *);
    void visitPreIfndefDirective_Pre (Puma::PreIfndefDirective *);
    void visitPreElifDirective_Pre (Puma::PreElifDirective *);
    void visitPreElseDirective_Pre (Puma::PreElseDirective *);
};

#endif
