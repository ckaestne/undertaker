/*
 *   libziz - parse c preprocessor files
 * Copyright (C) 2010 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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


#include "Ziz.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>


using namespace Ziz;

// Ziz

File* Parser::Parse(const std::string file) {
    // Open and read in the specified input file.
    std::ifstream instream(file.c_str());
    if (!instream.is_open())
        throw ZizException((std::string("could not open file: ") + file).c_str());

    try {
        // create a file object, let _p_curBlockContainer point to it
        _p_file = new File;
        _p_curBlockContainer = _p_file;

        instream.unsetf(std::ios::skipws);
        std::string instr = std::string(
                std::istreambuf_iterator<char>(instream.rdbuf()),
                std::istreambuf_iterator<char>());

        // Tokenize the input data into C++ tokens using the C++ lexer
        position_type pos(file.c_str());
        lexer_type it = lexer_type(instr.begin(), instr.end(), pos,
                boost::wave::language_support(
                    boost::wave::support_cpp|boost::wave::support_option_long_long));
        lexer_type end = lexer_type();

        // Let's go parsing!
        while (it != end) {
            _prevPos = _curPos;
            _curPos = (*it).get_position();

            boost::wave::token_id id = boost::wave::token_id(*it);
            switch (id) {
                case boost::wave::T_PP_IF:
                    HandleIF(it);
                    break;
                case boost::wave::T_PP_IFDEF:
                    HandleIFDEF(it);
                    break;
                case boost::wave::T_PP_IFNDEF:
                    HandleIFNDEF(it);
                    break;
                case boost::wave::T_PP_ELSE:
                    HandleELSE(it);
                    break;
                case boost::wave::T_PP_ELIF:
                    HandleELIF(it);
                    break;
                case boost::wave::T_PP_ENDIF:
                    HandleENDIF(it);
                    break;
                case boost::wave::T_PP_DEFINE:
                    HandleDEFINE(it);
                    break;
                case boost::wave::T_PP_UNDEF:
                    HandleUNDEF(it);
                    break;
                default:
                    HandleToken(it);
                    break;
            }
            // The lexer iterator might have gone further in the Handle*()
            // functions, so we'll check for EOF.
            if (it != end)
                ++it;
        } // end of parsing

        // finish the very last block
        FinishSaveCurrentCodeBlock();
        FinishSaveCurrentConditionalBlock(it);

    } // try

    catch (boost::wave::cpplexer::lexing_exception const& e) {
        // some lexing error
        //ss << "[PreProcCpp] Process() lexing error: " << e.file_name() << "("
            //<< e.line_no() << "): " << e.description() << endl;
        throw e;
    }
    catch (...) {
        throw ZizException("WTF");
    }

    return _p_file;
}

void Parser::HandleOpeningCondBlock(lexer_type& lexer) {
    assert(_p_curBlockContainer != nullptr);

    FinishSaveCurrentCodeBlock();

    ConditionalBlock* pBlock =
        _p_file->CreateConditionalBlock(_condBlockStack.size(), _curPos,
                                        _p_curBlockContainer, lexer);

    _p_curBlockContainer = pBlock;
    _condBlockStack.push(pBlock);
}

void Parser::HandleElseBlock(lexer_type& lexer) {
    assert(_p_curBlockContainer != nullptr);

    FinishSaveCurrentCodeBlock();

    // The #if/#elif block belonging to this #else/#elif is on top of the block stack.
    if (_condBlockStack.empty()){
        // TODO: add _curPos info to msg
        throw ZizException(std::string(
                    "no open #if block for #else/#elif").c_str());
    }
    ConditionalBlock* pPrevIfBlock = _condBlockStack.top();
    _condBlockStack.pop();

    pPrevIfBlock->SetEnd(_curPos);
    // Add the previous #if/#elif block to current blocklist (either file or
    // inner block).
    if (_condBlockStack.empty()) {
        _p_curBlockContainer = _p_file;   // we just left a top-level block
    } else {
        _p_curBlockContainer = _condBlockStack.top();
    }
    _p_curBlockContainer->push_back(pPrevIfBlock);

    // Create the actual #else/#elif block, set previous silbling, set this
    // #else/#elif block as current block container and push to block stack.
    ConditionalBlock* pElseBlock =
        _p_file->CreateConditionalBlock(_condBlockStack.size(), _curPos,
                                        _p_curBlockContainer, lexer);
    pElseBlock->SetPrevSibling(pPrevIfBlock);
    _p_curBlockContainer = pElseBlock;
    _condBlockStack.push(pElseBlock);
}

void Parser::HandleIF(lexer_type& lexer) {
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleIFDEF(lexer_type& lexer) {
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleIFNDEF(lexer_type& lexer) {
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleELSE(lexer_type& lexer) {
    HandleElseBlock(lexer);
}

void Parser::HandleELIF(lexer_type& lexer) {
    HandleElseBlock(lexer);
}

void Parser::HandleDEFINE(lexer_type& lexer) {
    HandleDefines(true,lexer);
}

void Parser::HandleUNDEF(lexer_type& lexer) {
    HandleDefines(false, lexer);
}

void Parser::HandleDefines(bool define, lexer_type& lexer) {
    /* needed:
     *  id (File._defines),
     *  _flag (next token),
     *  _position (current position),
     *  _block (current block),
     *  _define (define)
     */

    if (_p_curCodeBlock == nullptr)
        _p_curCodeBlock = _p_file->CreateCodeBlock(_condBlockStack.size(),
                                           _curPos, _p_curBlockContainer);

    // skip whitespace
    while (boost::wave::token_id(*lexer) == boost::wave::T_SPACE
           || boost::wave::token_id(*lexer) == boost::wave::T_SPACE2) {
        ++lexer;
    }

    if (define)
        assert(boost::wave::token_id(*lexer) == boost::wave::T_PP_DEFINE);
    else
        assert(boost::wave::token_id(*lexer) == boost::wave::T_PP_UNDEF);

    _p_curCodeBlock->AppendContent(lexer->get_value());
    lexer++;

    _p_curCodeBlock->AppendContent(std::string(" "));

    // skip whitespace
    while (boost::wave::token_id(*lexer) == boost::wave::T_SPACE
           || boost::wave::token_id(*lexer) == boost::wave::T_SPACE2) {
        ++lexer;
    }

    _p_curCodeBlock->AppendContent(lexer->get_value());

    std::stringstream flag;
    flag << lexer->get_value();

    _p_file->CreateDefine(_condBlockStack.size(), flag.str(), _curPos,
                          _p_curBlockContainer ? _p_curBlockContainer : _p_file,
                          define);

    //this while just moves lexer to the next line; it takes macros spanning several lines into consideration
    while (boost::wave::token_id(*lexer) != boost::wave::T_NEWLINE) {
        if (boost::wave::token_id(*lexer) == boost::wave::T_CONTLINE) {
            lexer++; // the '//' character
        }
        lexer++;
    }
    _p_curCodeBlock->AppendContent(std::string("\n"));

#if 0
// FIXME this breaks when DEBUG is defined, pos and id have to be readout from the lexer
#ifdef DEBUG
    std::cout << (define ? "#define " : "#undef ") << flag.str()
        << " at: " << pos
        << ", in B" << id
        << std::endl;
#endif
#endif
}

void Parser::HandleENDIF(lexer_type& lexer) {
    //std::cerr << "HandleENDIF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();
    FinishSaveCurrentConditionalBlock(lexer);
}


void Parser::HandleToken(lexer_type& lexer) {
    assert(_p_curBlockContainer != nullptr);

    /*
    boost::wave::token_id id = boost::wave::token_id(*lexer);
    std::cerr << "HandleToken() "
        << boost::wave::get_token_name(id) << " | "
        << lexer->get_value() << std::endl;
    */

    if (_p_curCodeBlock == nullptr)
        _p_curCodeBlock = _p_file->CreateCodeBlock(_condBlockStack.size(),
                                           _curPos, _p_curBlockContainer);
    _p_curCodeBlock->AppendContent(lexer->get_value());
}


void Parser::FinishSaveCurrentCodeBlock() {
    assert(_p_curBlockContainer != nullptr);

    if (_p_curCodeBlock == nullptr)
        return;

    _p_curCodeBlock->SetEnd(_prevPos);
    _p_curBlockContainer->push_back(_p_curCodeBlock);
    _p_curCodeBlock = nullptr;
}

void Parser::FinishSaveCurrentConditionalBlock(lexer_type& lexer) {
    assert(_p_curBlockContainer != nullptr);

    if (_condBlockStack.empty()) {
#ifdef DEBUG
        std::cerr << "I: FinishSaveCurrentConditionalBlock with empty block stack"
                  << std::endl;
#endif
        return;
    }

    // Pop current open ConditionalBlock from stack
    ConditionalBlock* pCurBlock = _condBlockStack.top();
    _condBlockStack.pop();

    // Finish the ConditionalBlock
    // read in whole condition until end of line
    pCurBlock->SetEnd(_curPos);

    lexer_type end = lexer_type();
    while (lexer != end
            && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)
            && boost::wave::token_id(*lexer) != boost::wave::T_CCOMMENT
            && boost::wave::token_id(*lexer) != boost::wave::T_CPPCOMMENT) {
        pCurBlock->AppendFooter(lexer->get_value());
        ++lexer;
    }
    if (lexer != end) {
        // we reached an EOL, not EOF, so include that token in the footer
        pCurBlock->AppendFooter(lexer->get_value());
    }

    if (_condBlockStack.empty()) {
        _p_curBlockContainer = _p_file;   // we just left a top-level block
    } else {
        _p_curBlockContainer = _condBlockStack.top();
    }

    // add this block to current blocklist (either file or inner block)
    _p_curBlockContainer->push_back(pCurBlock);
}


// File

CodeBlock*
File::CreateCodeBlock(int depth, position_type startPos, BlockContainer* pbc) {
    assert(pbc != nullptr);
    return new CodeBlock(_blocks++, depth, startPos, pbc);
}

ConditionalBlock*
File::CreateConditionalBlock(int depth, position_type startPos,
                             BlockContainer* pbc, lexer_type& lexer) {
    assert(pbc != nullptr);  // a parent block container is always needed

    lexer_type end = lexer_type();
    assert(lexer != end); // lexer has to point to first token (#if, #else, ...)

    // create the ConditionalBlock
    ConditionalBlock* pCurBlock =
        new ConditionalBlock(_cond_blocks++, depth, startPos, *lexer, pbc);

    // store the very first token (#if, #else, ...) in the header, but not in
    // the expression token vector
    pCurBlock->AppendHeader(lexer->get_value());
    lexer++;

    // Read in whole condition until end of line.
    // CPPComments include the newline ("// comment\n"), so no EOLToken will
    // follow them.
    while (lexer != end && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)
            && boost::wave::token_id(*lexer) != boost::wave::T_CPPCOMMENT) {

        pCurBlock->AppendHeader(lexer->get_value());    // textual value

        // all meaningful tokens go into the expression token vector
        // comments and whitespaces are skipped
        if (!IS_CATEGORY(*lexer, boost::wave::WhiteSpaceTokenType)) {
            pCurBlock->AddToExpression(*lexer);
        }

        ++lexer;                                        // next token
    }
    if (lexer != end) {
        // we reached an EOL, not EOF, so include that token in the header
        pCurBlock->AppendHeader(lexer->get_value());
    }

    return pCurBlock;
}

void File::CreateDefine(int depth, std::string flag, position_type pos,
                        BlockContainer* block, bool define) {
    Define* r =  new Define(_defines++, depth, pos, block, flag, define);
    _defines_map[flag].push_back(r);
    block->push_back(r);
}


// ConditionalBlock

condblock_type ConditionalBlock::CondBlockType() const {
    boost::wave::token_id id = boost::wave::token_id(_type);
    std::string tstr(boost::wave::get_token_name(id).c_str());

    if (tstr.compare("PP_IF") == 0)
        return Ifdef;

    if (tstr.compare("PP_IFDEF") == 0)
        return Ifdef;

    if (tstr.compare("PP_IFNDEF") == 0)
        return Ifndef;

    if (tstr.compare("PP_ELIF") == 0)
        return Elif;

    if (tstr.compare("PP_ELSE") == 0)
        return Else;

    std::cerr << "unknown type: " << tstr.c_str() << std::endl;
    assert(false);
}

std::string ConditionalBlock::TokenStr() const {
    boost::wave::token_id id = boost::wave::token_id(_type);
    return std::string(boost::wave::get_token_name(id).c_str());
}

std::string ConditionalBlock::ExpressionStr() const {
    std::stringstream ss;
    std::vector<token_type>::const_iterator tit; // for _t_oken _it_erator ;-)

    bool first = true;
    for (tit = _expression.begin(); tit != _expression.end(); ++tit) {
        if (first) {
            first = false;
        } else {
            ss << " ";
        }
        ss << tit->get_value();
    }

    return ss.str();
}

ConditionalBlock *ConditionalBlock::ParentCondBlock() const {
    BlockContainer *p = Parent();
    return dynamic_cast<ConditionalBlock*>(p);
}
