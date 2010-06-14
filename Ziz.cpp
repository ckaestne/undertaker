#include "Ziz.h"

#include <fstream>
#include <iostream>
#include <iomanip>


// Ziz

CPPFile Ziz::Parse(std::string file)
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
        // FIXME: blocks at top/bottom of file are ignored, stack not really used, blahrghl
        while (it != end) {
            _prevPos = _curPos;
            _curPos = (*it).get_position();

            boost::wave::token_id id = boost::wave::token_id(*it);
            switch (id) {
                case boost::wave::T_PP_IFDEF:
                    HandleIFDEF(it);
                    break;

                case boost::wave::T_PP_ENDIF:
                    HandleENDIF(it);
                    break;

                default:
                    HandleToken(it);
                    break;
            }

#if 0 // 2nd version - bullshit
            if (boost::wave::token_id(*it) == boost::wave::T_PP_IFDEF) {
                if (!_blockStack.empty()) {
                    // close current block if one exists, save it in blocklist
                    if (_p_curBlock != NULL) {
                        _p_curBlock->SetEnd(_prevPos);
                        _p_curBlock->SetContent(_curBlockContent.str());
                        _tree._blocklist.push_back(*_p_curBlock);
                        delete _p_curBlock;
                    }
                }

                _p_curBlock = new CPPBlock(_blockCounter++, *it, _curPos);
                _curBlockContent.str(""); // empty the stringstream
                _curBlockContent.clear();

                // read in whole condition until end of line
                _curBlockContent << it->get_value();     // store #ifdef token itself
                while (it != end && !IS_CATEGORY(*it, boost::wave::EOLTokenType)) {
                    _curBlockContent << it->get_value();
                    ++it;
                }

                // TODO: set father

                // push to condition stack
                _blockStack.push(*_p_curBlock);

            } else if (boost::wave::token_id(*it) == boost::wave::T_PP_ENDIF) {
                if (_p_curBlock == NULL) {
                    throw ZizException("#endif without open #if");
                }

                // read in until end of line
                _curBlockContent << it->get_value();     // store #endif token itself
                while (it != end) {
                    _curBlockContent << it->get_value();
                    if (IS_CATEGORY(*it, boost::wave::EOLTokenType))
                        break;
                    ++it;
                }
                _p_curBlock->SetEnd(_curPos);
                _p_curBlock->SetContent(_curBlockContent.str());
                _tree._blocklist.push_back(*_p_curBlock);
                delete _p_curBlock;

            } else {
                // "normal" token (no conditional CPP directive)
                if (_p_curBlock == NULL) {
                    _p_curBlock = new CPPBlock(_blockCounter++, *it, _curPos);
                    _curBlockContent.str(""); // empty the stringstream
                    _curBlockContent.clear();
                }
                _curBlockContent << it->get_value();
            }
#endif

#if 0 // 1st version - blah
            if (boost::wave::token_id(*it) == boost::wave::T_PP_IFDEF) {
                if (!_blockStack.empty()) {
                    // close current block if one exists, save it in blocklist
                    if (_p_curBlock != NULL) {
                        _p_curBlock->SetEnd(_prevPos);
                        _tree._blocklist.push_back(*_p_curBlock);
                        delete _p_curBlock;
                    }
                }

                _p_curBlock = new CPPBlock(_blockCounter++, *it, _curPos);

                // TODO: set father

                // read in whole condition until end of line
                std::stringstream line;
                line << it->get_value();            // store #ifdef token itself
                while (it != end && !IS_CATEGORY(*it, boost::wave::EOLTokenType)) {
                    line << it->get_value();
                    ++it;
                }
                _p_curBlock->SetContent(line.str());

                // push to condition stack
                _blockStack.push(*_p_curBlock);
            }

            else if (boost::wave::token_id(*it) == boost::wave::T_PP_ENDIF) {
                if (_p_curBlock == NULL) {
                    throw ZizException("#endif without open #if");
                }

                // finish the current block
                _p_curBlock->SetEnd(_prevPos);



                _blockStack.pop();


                CPPCondition cond_endif;
                cond_endif.directive = *it;

                // read in whole condition until end of line
                output.str("");             // empty the stringstream
                output.clear();
                output << it->get_value();  // store #ifdef token itself
                while (it != end && !IS_CATEGORY(*it, boost::wave::EOLTokenType)) {
                    output << it->get_value();
                    ++it;
                }
                cond_endif.line = output.str();

            }

            else {
                // "normal" token (no conditional CPP directive)
                output << it->get_value();
            }
#endif

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

    return _cppfile;
}

void Ziz::HandleToken(lexer_type& lexer)
{
    boost::wave::token_id id = boost::wave::token_id(*lexer);
    std::cerr << "HandleToken() "
        << boost::wave::get_token_name(id) << " | "
        << lexer->get_value() << std::endl;

    if (_p_curCodeBlock == NULL)
        _p_curCodeBlock = _cppfile.CreateCodeBlock(_curPos);
    _p_curCodeBlock->AppendContent(lexer->get_value());
}

void Ziz::HandleIFDEF(lexer_type& lexer)
{
    std::cerr << "HandleIFDEF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();

    if (_p_curConditionalBlock != NULL)
        _condBlockStack.push(_p_curConditionalBlock);

    _p_curBlockContainer = _p_curConditionalBlock
                         = _cppfile.CreateConditionalBlock(_curPos, lexer);
}

void Ziz::HandleENDIF(lexer_type& lexer)
{
    std::cerr << "HandleENDIF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();
    FinishSaveCurrentConditionalBlock(lexer);
}

void Ziz::FinishSaveCurrentCodeBlock()
{
    if (_p_curCodeBlock == NULL)
        return;

    _p_curCodeBlock->SetEnd(_prevPos);
    _cppfile.AddBlock(_p_curCodeBlock);
    _p_curCodeBlock = NULL;
}

void Ziz::FinishSaveCurrentConditionalBlock(lexer_type& lexer)
{
    if (_p_curConditionalBlock == NULL)
        return;

    // read in whole condition until end of line
    lexer_type end = lexer_type();
    if (lexer != end)
        _p_curConditionalBlock->AppendFooter(lexer->get_value()); // #endif itself
    while (lexer != end && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)) {
        _p_curConditionalBlock->AppendFooter(lexer->get_value());
        ++lexer;
    }

    // add this block to current blocklist (either cppfile or inner block)
    _p_curBlockContainer->AddBlock(_p_curConditionalBlock);

    if (! _condBlockStack.empty()) {        // we just left an inner block
        _p_curBlockContainer = _p_curConditionalBlock = _condBlockStack.top();
        _condBlockStack.pop();
    } else {
        _p_curBlockContainer = &_cppfile;   // we just left a top-level block
        _p_curConditionalBlock = NULL;
    }
}


// CPPFile

CodeBlock*
CPPFile::CreateCodeBlock(position_type s)
{
    return new CodeBlock(_blocks++, s);
}

ConditionalBlock*
CPPFile::CreateConditionalBlock(position_type startPos, lexer_type& lexer)
{
    ConditionalBlock* p_CB = new ConditionalBlock(_blocks++, startPos, *lexer);

    // read in whole condition until end of line
    p_CB->AppendHeader(lexer->get_value());     // store #ifdef token itself
    lexer_type end = lexer_type();
    while (lexer != end && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)) {
        p_CB->AppendHeader(lexer->get_value());
        ++lexer;
    }

    return p_CB;
}


// output operators

std::ostream & operator<<(std::ostream &stream, CPPFile const &t)
{
    std::vector<CPPBlock*> blocklist = t.InnerBlocks();
    std::vector<CPPBlock*>::const_iterator it;
    for (it = blocklist.begin(); it != blocklist.end(); ++it)
        stream << **it;
    return stream;
}

std::ostream & operator<<(std::ostream &stream, CPPBlock const &b)
{
    stream << "BEGIN BLOCK " << b.Id() << "\n";
    stream << "start: " << b.Start() << "\n";
    stream << "end: " << b.End() << "\n";
    stream << "END BLOCK " << b.Id() << std::endl;
    return stream;
}
