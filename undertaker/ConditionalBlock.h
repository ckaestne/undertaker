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

#ifndef _CONDITIONALBLOCK_H_
#define _CONDITIONALBLOCK_H_

#include <boost/regex.hpp>
#include "StringJoiner.h"
#include "ConfigurationModel.h"
#include "Ziz.h"


class ConditionalBlock;
class CppDefine;

class CppFile : public std::list<ConditionalBlock *> {
 public:
    //! \param filename file with cpp expressions to parse
    CppFile(const char *filename);
    ~CppFile();

    //! Check if the file was correctly parsed
    bool good() { return top_block ? true : false; };

    /**
     * The top block of an parsed file is an artificial block, which
     * doesn't represent an concrete Ziz::ConditionalBlock, but the
     * whole file.
     * \return top block
    */
    ConditionalBlock *topBlock() const { return top_block; };


    /**
     * \return map with defined symbol to define object
     */
    std::map<std::string, CppDefine*> * getDefines() { return &define_map; };

    //! \return filename given in the constructor
    std::string getFilename() const { return filename; };

    /**
     * \param postition format: "filename:line:pos"
     * \return innermost block at specific position
     */
    ConditionalBlock *getBlockAtPosition(const std::string &position);


    //! Functor that checks if a given symbol was touched by an define
    class ItemChecker : public ConfigurationModel::Checker {
    public:
        ItemChecker(CppFile *cf) : file(cf) {}
        bool operator()(const std::string &item) const;
    private:
        CppFile * file;
    };

    const ItemChecker *getChecker() const { return &checker;}

 private:
    ConditionalBlock* doZizWrap(ConditionalBlock *parent,
                                ConditionalBlock *prev,
                                Ziz::BlockContainer *container);

    void handleDefine(ConditionalBlock *parent, Ziz::Define *define);

 private:
    std::string filename;
    ConditionalBlock *top_block;
    std::map<std::string, CppDefine *> define_map;
    const CppFile::ItemChecker checker;
};

class ConditionalBlock : public std::list<ConditionalBlock *> {
 public:
    ConditionalBlock(CppFile *file, ConditionalBlock *parent,
                     ConditionalBlock *prev,
                     Ziz::ConditionalBlock *cb);

    ~ConditionalBlock() { delete _cb; delete cached_code_expression; }

    const Ziz::ConditionalBlock *ZizBlock() const { return _cb; }

    //! \return enclosing block or 0 if == cpp_file->topBlock()
    const ConditionalBlock * getParent() const { return _parent; }
    //! \return previous block on current level or 0 if first block on level
    const ConditionalBlock * getPrev() const { return _prev; }

    bool isIfBlock() const; //!< is if or ifdef block
    std::string ifdefExpression() { return _exp; };

    const std::string getName() const;
    std::string getConstraintsHelper(UniqueStringJoiner *and_clause = 0);
    std::string getCodeConstraints(UniqueStringJoiner *and_clause = 0,
                                  std::set<ConditionalBlock *> *visited = 0);

    void addDefine(CppDefine* define) { _defines.push_back(define); }

    CppFile * getFile() { return cpp_file; }

 private:
    CppFile * cpp_file;
    const Ziz::ConditionalBlock * _cb;
    const ConditionalBlock *_parent, *_prev;
    std::string _exp;

    std::string *cached_code_expression;

    std::list<CppDefine *> _defines;
};

class CppDefine {
public:
    CppDefine(ConditionalBlock *parent, Ziz::Define *ziz_define);

    void newDefine(ConditionalBlock *parent, Ziz::Define *ziz_define);

    std::string replaceDefinedSymbol(const std::string &exp);

    std::string getConstraints(UniqueStringJoiner *and_clause = 0,
                                  std::set<ConditionalBlock *> *visited = 0);
    bool containsDefinedSymbol(const std::string &exp);

private:
    std::set<std::string> isUndef;
    std::string actual_symbol; // The defined symbol will be replaced
                               // by this

    std::string defined_symbol; // The defined symbol

    std::deque <ConditionalBlock *> defined_in;
    std::list <std::string> defineExpressions;

    boost::regex replaceRegex;
};

#endif /* _CONDITIONALBLOCK_H_ */