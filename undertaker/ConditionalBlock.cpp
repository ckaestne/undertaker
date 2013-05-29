/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2013 Stefan Hengelein <stefan.hengelein@fau.de>
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

#include "StringJoiner.h"
#include "ConditionalBlock.h"
#include "ModelContainer.h"

#include "PumaConditionalBlock.h"
typedef PumaConditionalBlock ConditionalBlockImpl;
typedef PumaConditionalBlockBuilder ConditionalBlockImplBuilder;
#include <Puma/PreParser.h>

#include "SatChecker.h"
#include "BoolExpSymbolSet.h"

#include "Logging.h"

#include <boost/regex.hpp>
#include <set>

bool ConditionalBlock::useBlockWithFilename = false;

CppFile::CppFile(const char *f) : top_block(0), checker(this) {
    if (strncmp("./", f, 2))
        filename = f;
    else
        filename = f+2; // skip leading './'
    top_block = ConditionalBlockImpl::parse(f, this);
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


std::set<std::string> ConditionalBlock::itemsOfString(const std::string &str) {
    kconfig::BoolExp *e = kconfig::BoolExp::parseString(str);
    kconfig::BoolExpSymbolSet symset(e);
    delete e;
    return symset.getSymbolSet();
}


ConditionalBlock * CppFile::getBlockAtPosition(const std::string &position) {
    int line = lineFromPosition(position);

    ConditionalBlock *block = 0;
    int block_length = -1;

    // Iterate over all block
    for(iterator i = begin(); i != end(); ++i) {
        int begin = (*i)->lineStart();
        int last  = (*i)->lineEnd();

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

static ConditionalBlockImpl *createDummyElseBlock(ConditionalBlock *i,
        ConditionalBlock *parent, ConditionalBlock *prev) {
    ConditionalBlockImpl *superblock = dynamic_cast<ConditionalBlockImpl *>(i);
    if (!superblock) {
        logger << error << "failed to access the super-class of Conditionalblock" << std::endl;
        exit(1);
    }
    ConditionalBlockImplBuilder &builder = superblock->getBuilder();

    Puma::Token *tok = new Puma::Token(TOK_PRE_ELSE, Puma::Token::pre_id, "#else");
    Puma::PreTreeToken *ptok = new Puma::PreTreeToken(tok);

    Puma::Token *tok2 = new Puma::Token(TOK_PRE_ELSE, Puma::Token::pre_id, "");
    Puma::PreTreeToken *ptok2 = new Puma::PreTreeToken(tok2);

    Puma::PreElseDirective *node = new Puma::PreElseDirective(ptok, ptok2);
    unsigned long *nodeNum = builder.getNodeNum();

    ConditionalBlockImpl *newBlock = new ConditionalBlockImpl(i->getFile(), parent,
            prev, node, (*nodeNum)++, builder);
    newBlock->setDummyBlock();
    return newBlock;
}

void CppFile::decisionCoverage() {
#if 0
    logger << debug << "======== before TRANSFORMATION ========" << std::endl;
    this->topBlock()->printConditionalBlocks(0);
#endif
    this->topBlock()->processForDecisionCoverage();
#if 0
    logger << debug << "======== after TRANSFORMATION ========" << std::endl;
    this->topBlock()->printConditionalBlocks(0);
    printCppFile();
#endif
}

void CppFile::printCppFile() {
    logger << debug << "------ FILE ------" << std::endl;
    for (std::list<ConditionalBlock *>::iterator i = this->begin(); i != this->end(); ++i) {
        if ((*i)->ifdefExpression() == "") {
            if ((*i)->getName().compare("B00"))
                logger << debug << "ELSE " << (*i)->isElseBlock() << " "
                       << (*i)->getName() << " " << (*i) << std::endl;
        } else {
            logger << debug << (*i)->ifdefExpression() << " " << (*i)->isIfndefine()
                   << " " << (*i)->isIfBlock() << " " << (*i)->isElseIfBlock() << " "
                   << (*i)->getName() << " " << (*i) << std::endl;
        }
    }
    logger << debug << "------ END FILE ------" << std::endl;
}

void ConditionalBlock::insertBlockIntoFile(ConditionalBlock *prevBlock, ConditionalBlock *nblock,
        bool insertAfter) {
    CppFile *file = this->getFile();
    for (std::list<ConditionalBlock *>::iterator i = file->begin(); i != file->end(); ++i) {
        if ((*i) == prevBlock) {
            if (insertAfter)
                file->insert(++i, nblock);
            else
                file->insert(i, nblock);
            break;
        }
    }
}

void ConditionalBlock::processForDecisionCoverage() {
    for (std::list<ConditionalBlock *>::iterator i = this->begin(), prev = this->end();
            i != this->end(); prev = i, ++i) {

        // insert else when:  1. we are in an if-block 2. the previous block was a if / elseif
        if (prev != this->end() && (*i)->isIfBlock() &&
                ((*prev)->isIfBlock() || (*prev)->isElseIfBlock())) {
            ConditionalBlock *parent = const_cast<ConditionalBlock *>((*i)->_parent);
            ConditionalBlockImpl *nblock = createDummyElseBlock(*i, parent, *prev);
            parent->insert(i, nblock);
            // this inserts the Block also into the correct position in the CppFile List
            insertBlockIntoFile(*i, nblock);
        }
        // when the last element of the list is an if-expression
        if (*i == this->back() && ((*i)->isIfBlock() || (*i)->isElseIfBlock())) {
            ConditionalBlock *parent = const_cast<ConditionalBlock *>((*i)->_parent);
            ConditionalBlockImpl *nblock = createDummyElseBlock(*i, parent, *i);
            parent->push_back(nblock);
            // this inserts the Block also into the correct position in the CppFile List
            if (*i == this->getFile()->back())
                this->getFile()->push_back(nblock);
            else if ((*i)->size() > 0)
                insertBlockIntoFile((*i)->back(), nblock, true);
            else
                insertBlockIntoFile(*i, nblock, true);
        }
        if ((*i)->size() > 0)
            (*i)->processForDecisionCoverage();
    }
}

void ConditionalBlock::printConditionalBlocks(int indent) {
    for (std::list<ConditionalBlock *>::iterator i = this->begin(); i != this->end(); ++i) {
        if ((*i)->ifdefExpression() == "") {
            logger << debug << std::string(indent, ' ') << "ELSE " << (*i)->isElseBlock()
                   << " " << (*i)->getName() << " " << (*i) << " prev: "
                   << (*i)->getPrev() << std::endl;
        } else {
            logger << debug << std::string(indent, ' ') << (*i)->ifdefExpression()
                   << " " << (*i)->isIfndefine() << " " << (*i)->isIfBlock() << " "
                   << (*i)->isElseIfBlock() << " " << (*i)->getName() << " " << (*i)
                   << " prev: " << (*i)->getPrev() << std::endl;
        }
        if ((*i)->size() > 0)
            (*i)->printConditionalBlocks(indent + 4);
    }
}

void ConditionalBlock::lateConstructor() {
    if (_parent)  { // Not the toplevel block
        // extract expression
        _exp = ExpressionStr();
        if (isIfndefine())
            _exp = "! " + _exp;

        std::string::size_type pos = std::string::npos;
        while ((pos = _exp.find("defined")) != std::string::npos)
            _exp.erase(pos,7);

        /* Define Rewriting */
        for ( std::map<std::string, CppDefine *>::iterator i = cpp_file->getDefines()->begin();
              i != cpp_file->getDefines()->end(); ++i) {
            _exp = (*i).second->replaceDefinedSymbol(_exp);
        }
    }
}

std::string ConditionalBlock::getConstraintsHelper(UniqueStringJoiner *and_clause) {
    if (!_parent) return "B00"; // top_level block, represents file

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

std::string ConditionalBlock::normalize_filename(const char * name) {
    std::string normalized(name);

    for (std::string::iterator i = normalized.begin();
         i != normalized.end(); i++)
        if ((*i) == '/' || (*i) == '-' || (*i) == '+' || (*i) == ':')
            *i = '_';
    return normalized;
}

std::string ConditionalBlock::getCodeConstraints(UniqueStringJoiner *and_clause,
                                                 std::set<ConditionalBlock *> *visited) {
    UniqueStringJoiner sj; // on our stack
    std::stringstream fi; // file identifier
    ModelContainer *mc = ModelContainer::getInstance();
    bool join = false;

    fi << "FILE_";
    fi << normalize_filename(this->filename());


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

        if (!_parent) { // Toplevel block
            if (join == true) {
                /* When we are the toplevel call we ensure, that no
                element is added more than once to the and_clause,
                therefore we can disable the unique attribute within
                the StringJoiner to improve performance */
                and_clause->disableUniqueness();
            }

            // Add expressions for all blocks
            for (CppFile::iterator i = cpp_file->begin(); i != cpp_file->end(); ++i) {
                ConditionalBlock *block = (*i);
                block->getConstraintsHelper(and_clause);

            }
            /* Get all used defines */
            for (std::map<std::string, CppDefine *>::const_iterator def = cpp_file->getDefines()->begin();
                  def != cpp_file->getDefines()->end(); ++def) {
                const CppDefine *define = (*def).second;
                define->getConstraintsHelper(and_clause);
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
                if (define->containsDefinedSymbol(ExpressionStr())) {
                    define->getConstraints(and_clause, visited);
                }
            }
        }

        if (mc && mc->size() > 0) {
            StringJoiner file_joiner;

            file_joiner.push_back("(");
            file_joiner.push_back("B00");
            file_joiner.push_back("<->");
            file_joiner.push_back(fi.str());
            file_joiner.push_back(")");
            and_clause->push_back(file_joiner.join(" "));
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

    /* Was also defined here */
    defined_in.push_back(parent);

    // If actual define is an undef insert it into the undef map
    if (!define) {
        isUndef.insert(parent->getName());
    }

    // If actual Block is selected, we select or deselect the flag
    std::string right_side = (define ? "" : "!") + new_symbol;

    // Block defined -> new_symbol is active
    defineExpressions.push_back("(" + parent->getName() + " -> " + right_side + ")");

    // !block defined -> old symbol == new_symbol
    defineExpressions.push_back("(!" + parent->getName() + " -> (" + actual_symbol + " <-> " + new_symbol + "))");

    /* B --> B. */
    actual_symbol = new_symbol;

    const std::string symbolSpace = "([() ><&|!-]|^|$)";

    replaceRegex = boost::regex(symbolSpace + "(" + defined_symbol + ")" + symbolSpace,
                                boost::regex::perl);

}

std::string CppDefine::replaceDefinedSymbol(const std::string &exp) {
    std::string copy(exp);
    boost::match_results<std::string::iterator> what;
    boost::match_flag_type flags = boost::match_default;

    if (!strstr(exp.c_str(), defined_symbol.c_str()))
        return exp;

    while ( boost::regex_search(copy.begin(), copy.end(), what, replaceRegex, flags)) {
        copy.replace(what[2].first, what[2].second, actual_symbol);
    }
    return copy;
}

bool CppDefine::containsDefinedSymbol(const std::string &exp) {
    if (!strstr(exp.c_str(), defined_symbol.c_str()))
        return false;
    return boost::regex_search(exp, replaceRegex);
}


void CppDefine::getConstraintsHelper(UniqueStringJoiner *and_clause) const {
    for (std::list<std::string>::const_iterator i  = defineExpressions.begin(); i != defineExpressions.end(); ++i) {
        and_clause->push_back(*i);
    }
}


std::string CppDefine::getConstraints(UniqueStringJoiner *and_clause, std::set<ConditionalBlock *> *visited) {
    UniqueStringJoiner sj; // on our stack
    bool join = false;
    if (!and_clause) {
        and_clause = &sj; // We are the toplevel call
        join = true; // We also must return a true string
    }

    std::set<ConditionalBlock *> vs;
    if (!visited)
        visited = &vs;

    getConstraintsHelper(and_clause);

    for (std::deque <ConditionalBlock *>::iterator i = defined_in.begin(); i != defined_in.end(); i++) {
        // Not yet visited and not the toplevel block
        if (visited->count(*i) == 0 && (*i)->getParent() != 0) {
            (*i)->getCodeConstraints(and_clause, visited);
        }
    }

    return join ? and_clause->join("\n&& ") : "";
}
