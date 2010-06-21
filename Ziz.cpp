#include "Ziz.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <iomanip>


using namespace Ziz;

// Ziz

File Parser::Parse(std::string file)
{
    try {
        // Open and read in the specified input file.
        std::ifstream instream(file.c_str());
        if (!instream.is_open())
            throw ZizException((std::string("could not open file: ") + file).c_str());

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

    return _file;
}


void Parser::HandleOpeningCondBlock(lexer_type& lexer)
{
    FinishSaveCurrentCodeBlock();

    ConditionalBlock* pBlock =
        _file.CreateConditionalBlock(_condBlockStack.size(), _curPos,
                                     _p_curBlockContainer, lexer);

    _p_curBlockContainer = pBlock;
    _condBlockStack.push(pBlock);
}

void Parser::HandleElseBlock(lexer_type& lexer)
{
    FinishSaveCurrentCodeBlock();

    // The #if/#elif block belonging to this #else/#elif is on top of the block stack.
    if (_condBlockStack.empty()){
        // TODO: add _curPos info to msg
        throw ZizException(std::string(
                    "no open #if block for #else/#elif").c_str());
    }
    ConditionalBlock* pPrevIfBlock = _condBlockStack.top();
    _condBlockStack.pop();

    // Add the previous #if/#elif block to current blocklist (either file or
    // inner block).
    if (_condBlockStack.empty()) {
        _p_curBlockContainer = &_file;   // we just left a top-level block
    } else {
        _p_curBlockContainer = _condBlockStack.top();
    }
    _p_curBlockContainer->push_back(pPrevIfBlock);

    // Create the actual #else/#elif block, set previous silbling, set this
    // #else/#elif block as current block container and push to block stack.
    ConditionalBlock* pElseBlock =
        _file.CreateConditionalBlock(_condBlockStack.size(), _curPos,
                                     _p_curBlockContainer, lexer);
    pElseBlock->SetPrevSibling(pPrevIfBlock); 
    _p_curBlockContainer = pElseBlock;
    _condBlockStack.push(pElseBlock); 
}


void Parser::HandleIF(lexer_type& lexer)
{
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleIFDEF(lexer_type& lexer)
{
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleIFNDEF(lexer_type& lexer)
{
    HandleOpeningCondBlock(lexer);
}

void Parser::HandleELSE(lexer_type& lexer)
{
    HandleElseBlock(lexer);
}

void Parser::HandleELIF(lexer_type& lexer)
{
    HandleElseBlock(lexer);
}

void Parser::HandleENDIF(lexer_type& lexer)
{
    //std::cerr << "HandleENDIF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();
    FinishSaveCurrentConditionalBlock(lexer);
}


void Parser::HandleToken(lexer_type& lexer)
{
    /*
    boost::wave::token_id id = boost::wave::token_id(*lexer);
    std::cerr << "HandleToken() "
        << boost::wave::get_token_name(id) << " | "
        << lexer->get_value() << std::endl;
    */

    if (_p_curCodeBlock == NULL)
        _p_curCodeBlock = _file.CreateCodeBlock(_condBlockStack.size(),
                                                _curPos, _p_curBlockContainer);
    _p_curCodeBlock->AppendContent(lexer->get_value());
}


void Parser::FinishSaveCurrentCodeBlock()
{
    if (_p_curCodeBlock == NULL)
        return;

    _p_curCodeBlock->SetEnd(_prevPos);
    _p_curBlockContainer->push_back(_p_curCodeBlock);
    _p_curCodeBlock = NULL;
}

void Parser::FinishSaveCurrentConditionalBlock(lexer_type& lexer)
{
    if (_condBlockStack.empty()) {
        std::cerr << "FinishSaveCurrentConditionalBlock with empty block stack"
                  << std::endl;
        return;
    }

    // Pop current open ConditionalBlock from stack
    ConditionalBlock* pCurBlock = _condBlockStack.top();
    _condBlockStack.pop();

    // Finish the ConditionalBlock
    // read in whole condition until end of line
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
        _p_curBlockContainer = &_file;   // we just left a top-level block
    } else {
        _p_curBlockContainer = _condBlockStack.top();
    }

    // add this block to current blocklist (either file or inner block)
    _p_curBlockContainer->push_back(pCurBlock);
}



// File

CodeBlock*
File::CreateCodeBlock(int depth, position_type startPos, BlockContainer* pbc)
{
    assert(pbc != NULL);
    return new CodeBlock(_blocks++, depth, startPos, pbc);
}

ConditionalBlock*
File::CreateConditionalBlock(int depth, position_type startPos,
                             BlockContainer* pbc, lexer_type& lexer)
{
    assert(pbc != NULL);
    ConditionalBlock* pCurBlock =
        new ConditionalBlock(_blocks++, depth, startPos, *lexer, pbc);

    // read in whole condition until end of line
    lexer_type end = lexer_type();
    while (lexer != end
            && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)
            && boost::wave::token_id(*lexer) != boost::wave::T_CCOMMENT
            && boost::wave::token_id(*lexer) != boost::wave::T_CPPCOMMENT) {
        pCurBlock->AppendHeader(lexer->get_value());    // textual value

        // build the expression
        if (IS_CATEGORY(*lexer, boost::wave::IdentifierTokenType))
            pCurBlock->AddToExpression(*lexer);

        ++lexer;                                        // next token
    }
    if (lexer != end) {
        // we reached an EOL, not EOF, so include that token in the header
        pCurBlock->AppendHeader(lexer->get_value());
    }

    return pCurBlock;
}


// ConditionalBlock

std::string ConditionalBlock::TokenStr() const
{
    boost::wave::token_id id = boost::wave::token_id(_type);
    return std::string(boost::wave::get_token_name(id).c_str());
}

