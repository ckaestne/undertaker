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


#ifndef _ZIZ_CONDITIONAL_BLOCK_H_
#define _ZIZ_CONDITIONAL_BLOCK_H_

#include <boost/regex.hpp>
#include "StringJoiner.h"
#include "ConfigurationModel.h"
#include "ConditionalBlock.h"
#include "Ziz.h"



class ZizConditionalBlock : public ConditionalBlock {
 public:
    ZizConditionalBlock(CppFile *file, ConditionalBlock *parent,
                        ConditionalBlock *prev,
                        Ziz::ConditionalBlock *cb) :
        ConditionalBlock(file, parent, prev), _cb(cb) {
        lateConstructor();
    };

    ~ZizConditionalBlock() { delete _cb; }

    //! \return parse file and return top block
    static ConditionalBlock *parse(const char *filename,
                                   CppFile *cpp_file);

    /// @{
    //! location related accessors
    const char *filename()   const { return _cb->Start().get_file().c_str(); }
    unsigned int lineStart() const { return _cb->Start().get_line(); }
    unsigned int colStart()  const { return _cb->Start().get_column(); }
    unsigned int lineEnd()   const { return _cb->End().get_line(); }
    unsigned int colEnd()    const { return _cb->End().get_column(); }
    /// @}

    std::string ExpressionStr() const { return _cb->ExpressionStr(); }

    bool isIfBlock() const; //!< is if or ifdef block
    bool isIfndefine() const; //!< is ifndef block
    const std::string getName() const;

 private:
    static ConditionalBlock* doZizWrap(CppFile *file,
                                       ConditionalBlock *parent,
                                       ConditionalBlock *prev,
                                       Ziz::BlockContainer *container);


    const Ziz::ConditionalBlock * _cb;
};

#endif /* _ZIZ_CONDITIONAL_BLOCK_H_ */
