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
#include "ConditionalBlock.h"

CppFile::CppFile(const char *filename) : filename(filename), top_block(0), checker(this) {
    Ziz::Parser parser;
    Ziz::File *file;

    try {
        file = parser.Parse(filename);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
        return;
    } catch(...) {
        std::cerr << "Failed to parse '" << filename << "'" <<std::endl;
        return;
    }

    top_block = doZizWrap(0, 0, file);

    delete file;
}

CppFile::~CppFile(){
    /* Delete the toplevel block */

    delete topBlock();
    // We are an list of ConditionalBlocks
    for (iterator i = begin(); i != end(); ++i) {
        delete (*i);
    }
    // Remove also all defines
    
    for (std::map<std::string, CppDefine *>::iterator i = getDefines()->begin(); 
         i != getDefines()->end(); ++i) {
        delete (*i).second;
    }
}

bool CppFile::ItemChecker::operator()(const std::string &item) const {
    std::map<std::string, CppDefine*> *defines =  file->getDefines();
    return defines->find(item.substr(0, item.find('.'))) == defines->end();
}

ConditionalBlock * CppFile::doZizWrap(ConditionalBlock *parent,
                                      ConditionalBlock *prev,
                                      Ziz::BlockContainer *container) {
    Ziz::ConditionalBlock * cond = 0;
    // Only the top level block isn't a ConditionalBlock
    if (container->ContainerType() == Ziz::InnerBlock)
        cond = dynamic_cast<Ziz::ConditionalBlock *>(container);

    ConditionalBlock * block = new ConditionalBlock(this, parent, prev, cond);

    if (cond) // We are an inner block, so add the block to the list
        push_back(block);

    Ziz::BlockContainer::iterator i;
    ConditionalBlock * new_prev = 0;
    for (i = container->begin(); i != container->end(); i++) {
        Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
        Ziz::Define *define = dynamic_cast<Ziz::Define*>(*i);

        /* It is important to put the define rewriting at exactly this
           point, so the defines are handled in the right order */

        if (define)  {
            std::map<std::string, CppDefine *>::iterator i = define_map.find(define->getFlag());
            if (i == define_map.end()) {
                // First Define for this item, that every occured
                define_map[define->getFlag()] = new CppDefine(block, define->isDefine(), define->getFlag());
            } else {
                (*i).second->newDefine(block, define->isDefine());
            }

            block->addDefine(define_map[define->getFlag()]);
            /* Remove define because it's never used anymore */
            delete define;
            continue;
        } else if (cb) { // Condtional block
            /* Go recurive into the block tree and add it the generated
               block */
            new_prev = doZizWrap(block, new_prev, cb);
            block->push_back( new_prev );
        } else {
            /* Must be a code block, we simply free those */
            delete *i;
        }
    }

    return block;
}

static int lineFromPosition(std::string line) {
    // INPUT: foo:121:2
    // OUTPUT: 121

    size_t start = line.find_first_of(':');
    assert (start != line.npos);
    size_t stop = line.find_first_of(':', start);
    assert (stop != line.npos);

    std::stringstream ss(line.substr(start+1, stop));
    int i;
    ss >> i;
    return i;
}

ConditionalBlock *
CppFile::getBlockAtPosition(const std::string &position) {
    unsigned int line = lineFromPosition(position);

    ConditionalBlock *block = 0;
    int block_length = -1;

    // Iterate over all block
    for(iterator i = begin(); i != end(); ++i) {
        unsigned int begin = (*i)->lineStart();
        unsigned int last  = (*i)->lineEnd();

        if (last < begin) continue;
        /* Found a short block, using this one */
        if ((((last - begin) < block_length) || block_length == -1)
            && begin < line
            && line < last) {
            block = *i;
            block_length = last - begin;
        }
    }

    return block;
}


ConditionalBlock::ConditionalBlock(CppFile *file,
                                   ConditionalBlock *parent,
                                   ConditionalBlock *prev,
                                   Ziz::ConditionalBlock *cb)
    : cpp_file(file), _cb(cb), _parent(parent), _prev(prev),
      cached_code_expression(0) {

    if (parent)  { // Not the toplevel block
        // extract expression
        _exp = this->ExpressionStr();
        if (_cb->CondBlockType() == Ziz::Ifndef)
            _exp = "! " + _exp;

        std::string::size_type pos = std::string::npos;
        while ((pos = _exp.find("defined")) != std::string::npos)
            _exp.erase(pos,7);

        /* Define Rewriting */
        for ( std::map<std::string, CppDefine *>::iterator i = file->getDefines()->begin();
              i != file->getDefines()->end(); ++i) {
            _exp = (*i).second->replaceDefinedSymbol(_exp);
        }
    }
}

const std::string ConditionalBlock::getName() const {
    if (!_cb) return "B00"; // top level block, represents file

    return "B" + boost::lexical_cast<std::string>(_cb->Id());
}


bool ConditionalBlock::isIfBlock() const {
    if (!_cb) return true; // the top_level block is an implicit ifdef
                           // block

    if (_cb->CondBlockType() == Ziz::If
        || _cb->CondBlockType() == Ziz::Ifdef
        || _cb->CondBlockType() == Ziz::Ifndef)
        return true;
    return false;
}

std::string ConditionalBlock::getConstraintsHelper(UniqueStringJoiner *and_clause) {
    if (!_cb) return "B00"; // top_level block, represents file

    UniqueStringJoiner sj; // on our stack
    bool join = false;
    if (!and_clause) {
        and_clause = &sj; // We are the toplevel call
        join = true; // We also must return a true string
    }

    StringJoiner innerClause, predecessors;

    if (_parent != cpp_file->topBlock())
        innerClause.push_back(_parent->getName());

    innerClause.push_back(ifdefExpression());

    const ConditionalBlock *block = this;

    while(block) {
        // #ifdef reached
        if (block->isIfBlock()) break;
        block = block->getPrev();

        predecessors.push_back(block->getName());
        assert(block);
    }


   if (predecessors.size() > 0)
       innerClause.push_back("( ! (" + predecessors.join(" || ") + ") )");


   and_clause->push_back( "( " + getName() + " <-> " + innerClause.join(" && ") + " )");

   return join ? and_clause->join(" && ") : "";
}


std::string ConditionalBlock::getCodeConstraints(UniqueStringJoiner *and_clause, std::set<ConditionalBlock *> *visited) {
    UniqueStringJoiner sj; // on our stack
    bool join = false;
    if (!and_clause) {
        if (cached_code_expression)
            return *cached_code_expression;
        and_clause = &sj; // We are the toplevel call
        join = true; // We also must return a true string
    }

    std::set<ConditionalBlock *> vs;
    if (!visited)
        visited = &vs;

    if (visited->count(this) == 0 ) {
        // Mark our node as visited
        visited->insert(this);

        if (!_cb) { // Toplevel block
            // Add expressions for all blocks
            for (CppFile::iterator i = cpp_file->begin(); i != cpp_file->end(); ++i) {
                (*i)->getConstraintsHelper(and_clause);
                 /* Get all used defines */
                 for ( std::map<std::string, CppDefine *>::iterator def = cpp_file->getDefines()->begin();
                       def != cpp_file->getDefines()->end(); ++def) {
                     CppDefine *define = (*def).second;
                     if (define->containsDefinedSymbol((*i)->ExpressionStr())) {
                         define->getConstraints(and_clause, visited);
                     }
                 }
            }
            and_clause->push_back("B00");
        } else {
            const ConditionalBlock *block = this;
            const_cast<ConditionalBlock *>(block)->getConstraintsHelper(and_clause);

            if (block->isIfBlock())
                block = block->getParent();
            else
                block = block->getPrev();

            if (block && block != cpp_file->topBlock()) {
                const_cast<ConditionalBlock *>(block)->getCodeConstraints(and_clause, visited);

            }
            and_clause->push_back("B00");
            for ( std::map<std::string, CppDefine *>::iterator i = cpp_file->getDefines()->begin();
                  i != cpp_file->getDefines()->end(); ++i) {
                CppDefine *define = (*i).second;
                if (define->containsDefinedSymbol(_cb->ExpressionStr())) {
                    define->getConstraints(and_clause, visited);
                }
            }
        }
    }

    if (!cached_code_expression) {
        cached_code_expression = new std::string(and_clause->join("\n&& "));
    }

    // Do the join of the and clause only if we are the toplevel clause
    return join ? and_clause->join("\n&& ") : "";
}

CppDefine::CppDefine(ConditionalBlock *defined_in, bool define, const std::string &id) 
    : actual_symbol(id), defined_symbol(id) {
    newDefine(defined_in, define);
}

void
CppDefine::newDefine(ConditionalBlock *parent, bool define) {
    const char *rewriteToken = ".";
    std::string new_symbol = actual_symbol + rewriteToken;
    UniqueStringJoiner oldDefineBlocks;

    /* Was also defined here */
    defined_in.push_back(parent);

    // If actual define is an undef insert it into the undef map
    if (!define) {
        isUndef.insert(parent->getName());
    }


    for (std::deque <ConditionalBlock *>::reverse_iterator i = defined_in.rbegin(); i != defined_in.rend(); i++) {
        if (oldDefineBlocks.size() > 0) {
            std::string notPredecessors = oldDefineBlocks.join(" || ");

            std::string right_side = (isUndef.count((*i)->getName()) > 0 ? "!" : "") + new_symbol;

            defineExpressions.push_back("(( " + (*i)->getName() + " && !(" + notPredecessors + ")) -> " + right_side + ")");
        }

        oldDefineBlocks.push_front((*i)->getName());
    }

    if (oldDefineBlocks.size() == 0) {
        // This is the first define for this symbol
        oldDefineBlocks.push_back("B00");
    }

    // If actual Block is selected, we select or deselect the flag
    std::string right_side = (define ? "" : "!") + new_symbol;

    defineExpressions.push_back("(" + parent->getName() + " -> " + right_side + ")");

    //  (( B. && !(B5 || B0) ) -> B.. )
    std::string oldDefines = oldDefineBlocks.join(" || ");
    defineExpressions.push_back("((" + actual_symbol + "  && !(" + oldDefines + ")) -> "
                                + new_symbol + ")");
    //  (( B.. && !(B5 || B0) ) -> B. )
    defineExpressions.push_back("((" + new_symbol + " && !(" + oldDefines + ")) -> "
                                + actual_symbol + " )");

    // for (std::list<std::string>::iterator i  = defineExpressions.begin(); i != defineExpressions.end(); ++i) {
    //     std::cout << *i << std::endl;
    // }
    // std::cout << "----" << std::endl;


    /* B --> B. */
    actual_symbol = new_symbol;

    const std::string symbolSpace = "([() ><&|!-]|^|$)";

    replaceRegex = boost::regex(symbolSpace + "(" + defined_symbol + ")" + symbolSpace,
                                boost::regex::perl);

}

std::string
CppDefine::replaceDefinedSymbol(const std::string &exp) {
    std::string copy(exp);
    boost::match_results<std::string::iterator> what;
    boost::match_flag_type flags = boost::match_default;

    while ( boost::regex_search(copy.begin(), copy.end(), what, replaceRegex, flags)) {
        copy.replace(what[2].first, what[2].second, actual_symbol);
    }
    return copy;
}

bool
CppDefine::containsDefinedSymbol(const std::string &exp) {
    return boost::regex_search(exp, replaceRegex);
}


std::string
CppDefine::getConstraints(UniqueStringJoiner *and_clause, std::set<ConditionalBlock *> *visited) {
    UniqueStringJoiner sj; // on our stack
    bool join = false;
    if (!and_clause) {
        and_clause = &sj; // We are the toplevel call
        join = true; // We also must return a true string
    }

    std::set<ConditionalBlock *> vs;
    if (!visited)
        visited = &vs;

    for (std::list<std::string>::iterator i  = defineExpressions.begin(); i != defineExpressions.end(); ++i) {
        and_clause->push_back(*i);
    }

    for (std::deque <ConditionalBlock *>::iterator i = defined_in.begin(); i != defined_in.end(); i++) {
        // Not yet visited and not the toplevel block
        if (visited->count(*i) == 0 && (*i)->getParent() != 0) { 
            (*i)->getCodeConstraints(and_clause, visited);
        }
    }

    return join ? and_clause->join("\n&& ") : "";
}



