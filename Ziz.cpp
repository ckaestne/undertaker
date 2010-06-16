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

void Parser::HandleIFDEF(lexer_type& lexer)
{
    //std::cerr << "HandleIFDEF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();

    ConditionalBlock* pBlock =
        _file.CreateConditionalBlock(_condBlockStack.size(), _curPos,
                                     _p_curBlockContainer, lexer);

    _p_curBlockContainer = pBlock;
    _condBlockStack.push(pBlock);
}

void Parser::HandleENDIF(lexer_type& lexer)
{
    //std::cerr << "HandleENDIF() " << lexer->get_value() << std::endl;

    FinishSaveCurrentCodeBlock();
    FinishSaveCurrentConditionalBlock(lexer);
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
    while (lexer != end && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)) {
        pCurBlock->AppendFooter(lexer->get_value());
        ++lexer;
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
    ConditionalBlock* p_CB =
	new ConditionalBlock(_blocks++, depth, startPos, *lexer, pbc);

    // read in whole condition until end of line
    lexer_type end = lexer_type();
    while (lexer != end && !IS_CATEGORY(*lexer, boost::wave::EOLTokenType)) {
        p_CB->AppendHeader(lexer->get_value());
        ++lexer;
    }

    return p_CB;
}


// ConditionalBlock

std::string ConditionalBlock::TokenStr() const
{
    boost::wave::token_id id = boost::wave::token_id(_type);
    return std::string(boost::wave::get_token_name(id).c_str());
}


// helper functions

std::string Indent(int depth) {
    std::stringstream ss;
    ss << "|";
    for (int i=0; i < depth; i++)
        ss << "    |";
    return ss.str();
}


// output operators

// + short output (--short mode in zizler)
std::ostream & operator+(std::ostream &stream, File const &f)
{
    std::vector<Block*>::const_iterator it;
    for (it = f.begin(); it != f.end(); ++it)
        stream + **it;
    return stream;
}

std::ostream & operator+(std::ostream &stream, Block const &b)
{
    if (b.BlockType() == Code) {
        stream << "<code>\n";
    } else if (b.BlockType() == Conditional) {
        stream + dynamic_cast<ConditionalBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator+(std::ostream &stream, ConditionalBlock const &b)
{
    stream << "START BLOCK " << b.Id() << " [T=" << b.TokenStr() << "] "
           << "[H=" << b.Header() << "] [F=" << b.Footer() << "] [P=";

    BlockContainer* p_parent = b.Parent();
    assert(p_parent != NULL);
    if (p_parent->ContainerType() == OuterBlock) {
        stream << "<none>";
    } else if (p_parent->ContainerType() == InnerBlock) {
        stream << dynamic_cast<ConditionalBlock*>(p_parent)->Id();
    } else {
        assert(false);      // this may not happen
    }

    stream << "]\n";

    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream + **it;

    stream << "END BLOCK " << b.Id() << "\n";
    return stream;
}


// << normal output (default mode in zizler)

std::ostream & operator<<(std::ostream &stream, File const &f)
{
    std::vector<Block*>::const_iterator it;
    for (it = f.begin(); it != f.end(); ++it)
        stream << **it;
    return stream;
}

std::ostream & operator<<(std::ostream &stream, Block const &b)
{
    if (b.BlockType() == Code) {
        stream << dynamic_cast<CodeBlock const &>(b);
    } else if (b.BlockType() == Conditional) {
        stream << dynamic_cast<ConditionalBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator<<(std::ostream &stream, CodeBlock const &b)
{
    stream << "START CODE BLOCK " << b.Id() << "\n";
    stream << b.Content();
    stream << "END CODE BLOCK " << b.Id() << "\n";
    return stream;
}

std::ostream & operator<<(std::ostream &stream, ConditionalBlock const &b)
{
    stream << "START CONDITIONAL BLOCK " << b.Id() << "\n";
    stream << "token="      << b.TokenStr()    << "\n";
    stream << "header="     << b.Header()      << "\n";
    stream << "expression=" << b.Expression()  << "\n";
    stream << "footer="     << b.Footer()      << "\n";

    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream << **it;

    stream << "END CONDITIONAL BLOCK " << b.Id() << "\n";
    return stream;
}


// >> verbose output (--long mode in zizler)

std::ostream & operator>>(std::ostream &stream, File const &f)
{
    std::cout << "File has " << f.size() << " outer blocks\n\n";
    std::vector<Block*>::const_iterator it;
    for (it = f.begin(); it != f.end(); ++it)
        stream >> **it;
    return stream;
}

std::ostream & operator>>(std::ostream &stream, Block const &b)
{
    if (b.BlockType() == Code) {
        stream >> dynamic_cast<CodeBlock const &>(b);
    } else if (b.BlockType() == Conditional) {
        stream >> dynamic_cast<ConditionalBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator>>(std::ostream &stream, CodeBlock const &b)
{
    std::string indent = Indent(b.Depth());
    stream << indent
           << "-[" << b.Id() << "]---------------  START CODE BLOCK  "
           << "----------------[" << b.Id() << "]-\n";
    stream << indent << " start:       " << b.Start()   << "\n";
    stream << indent << " end:         " << b.End()     << "\n";
    stream << indent << " depth:       " << b.Depth()   << "\n";

    std::stringstream css(b.Content());
    std::string line;
    while (!css.eof()) {
        std::getline(css, line);
        stream << indent << " content:     " << line << "\n";
    }

    stream << indent
           << "-[" << b.Id() << "]---------------   END  CODE BLOCK  "
           << "----------------[" << b.Id() << "]-" << std::endl;
    return stream;
}

std::ostream & operator>>(std::ostream &stream, ConditionalBlock const &b)
{
    std::string indent = Indent(b.Depth());
    stream << indent
           << "=[" << b.Id() << "]============  START CONDITIONAL BLOCK  "
           << "============[" << b.Id() << "]=\n";
    stream << indent << " start:       " << b.Start()       << "\n";
    stream << indent << " end:         " << b.End()         << "\n";
    stream << indent << " depth:       " << b.Depth()       << "\n";
    stream << indent << " token:       " << b.TokenStr()    << "\n";
    stream << indent << " header:      " << b.Header()      << "\n";
    stream << indent << " expression:  " << b.Expression()  << "\n";
    stream << indent << " footer:      " << b.Footer()      << "\n";

    stream << indent <<" inner blocks: " << b.size() << "\n";
    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream >> **it;

    stream << indent
           << "=[" << b.Id() << "]============   END  CONDITIONAL BLOCK  "
           << "============[" << b.Id() << "]=" << std::endl;
    return stream;
}
