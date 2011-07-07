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

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include "StringJoiner.h"
#include "ZizConditionalBlock.h"


ConditionalBlock *ZizConditionalBlock::parse(const char *filename,
                                             CppFile *cpp_file) {
    Ziz::Parser parser;
    Ziz::File *file;

    try {
        file = parser.Parse(filename);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
        return 0;
    } catch(...) {
        std::cerr << "Failed to parse '" << filename << "'" <<std::endl;
        return 0;
    }

    ConditionalBlock *top_block = ZizConditionalBlock::doZizWrap(cpp_file, 0, 0, file);

    delete file;
    return top_block;
}

ConditionalBlock * ZizConditionalBlock::doZizWrap(CppFile *file,
                                                  ConditionalBlock *parent,
                                                  ConditionalBlock *prev,
                                                  Ziz::BlockContainer *container) {
    Ziz::ConditionalBlock * cond = 0;
    // Only the top level block isn't a ConditionalBlock
    if (container->ContainerType() == Ziz::InnerBlock)
        cond = dynamic_cast<Ziz::ConditionalBlock *>(container);

    ConditionalBlock * block = new ZizConditionalBlock(file, parent, prev, cond);
    block->lateConstructor();

    if (cond) // We are an inner block, so add the block to the list
        file->push_back(block);

    Ziz::BlockContainer::iterator i;
    ConditionalBlock * new_prev = 0;
    for (i = container->begin(); i != container->end(); i++) {
        Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
        Ziz::Define *define = dynamic_cast<Ziz::Define*>(*i);

        /* It is important to put the define rewriting at exactly this
           point, so the defines are handled in the right order */

        if (define)  {
            std::map<std::string, CppDefine *>::iterator i = file->define_map.find(define->getFlag());
            if (i == file->define_map.end()) {
                // First Define for this item, that every occured
                file->define_map[define->getFlag()] = new CppDefine(block, define->isDefine(), define->getFlag());
            } else {
                (*i).second->newDefine(block, define->isDefine());
            }

            block->addDefine(file->define_map[define->getFlag()]);
            /* Remove define because it's never used anymore */
            delete define;
            continue;
        } else if (cb) { // Condtional block
            /* Go recurive into the block tree and add it the generated
               block */
            new_prev = doZizWrap(file, block, new_prev, cb);
            block->push_back( new_prev );
        } else {
            /* Must be a code block, we simply free those */
            delete *i;
        }
    }

    return block;
}


const std::string ZizConditionalBlock::getName() const {
    if (!_parent) return "B00"; // top level block, represents file
    return "B" + boost::lexical_cast<std::string>(_cb->Id());
}

bool ZizConditionalBlock::isIfBlock() const {
    /* The toplevel block is an implicit ifdef block*/
    if (!_parent) return true;

    if (_cb->CondBlockType() == Ziz::If
        || _cb->CondBlockType() == Ziz::Ifdef
        || _cb->CondBlockType() == Ziz::Ifndef)
        return true;
    return false;
}

bool ZizConditionalBlock::isIfndefine() const {
    /* The toplevel block is an implicit ifdef block*/
    if (!_parent) return false;
    return (_cb->CondBlockType() == Ziz::Ifndef);
}
