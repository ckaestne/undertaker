#include "Zizler.h"

#include <cassert>
#include <iostream>

using namespace Ziz;


// output operators

// + short output (--short mode)
std::ostream & operator+(std::ostream &stream, File const * p_f)
{
    assert(p_f != NULL);
    std::vector<Block*>::const_iterator it;
    for (it = p_f->begin(); it != p_f->end(); ++it)
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
    stream << "START BLOCK " << b.Id() << " [T=" << b.TokenStr() << "] ";

    stream << "[E=" << b.ExpressionStr() << "] ";

    std::string header = b.Header();
    chomp(header);
    stream << "[H=" << header << "] ";

    std::string footer = b.Footer();
    chomp(footer);
    stream << "[F=" << footer << "] [P=";

    BlockContainer* p_parent = b.Parent();
    assert(p_parent != NULL);
    if (p_parent->ContainerType() == OuterBlock) {
        stream << "<none>";
    } else if (p_parent->ContainerType() == InnerBlock) {
        stream << dynamic_cast<ConditionalBlock*>(p_parent)->Id();
    } else {
        assert(false);      // this may not happen
    }

    stream << "] [PS=";
    ConditionalBlock* p_prevSibling = b.PrevSibling();
    if (p_prevSibling == NULL) {
        stream << "<none>";
    } else {
        stream << p_prevSibling->Id();
    }

    stream << "]\n";

    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream + **it;

    stream << "END BLOCK " << b.Id() << "\n";
    return stream;
}


// << normal output (default mode)
std::ostream & operator<<(std::ostream &stream, File const * p_f)
{
    assert(p_f != NULL);
    std::vector<Block*>::const_iterator it;
    for (it = p_f->begin(); it != p_f->end(); ++it)
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
    //stream << "expression=" << b.Expression()  << "\n"; // FIXME
    stream << "footer="     << b.Footer()      << "\n";

    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream << **it;

    stream << "END CONDITIONAL BLOCK " << b.Id() << "\n";
    return stream;
}


// >> verbose output (--long mode)
std::ostream & operator>>(std::ostream &stream, File const * p_f)
{
    assert(p_f != NULL);
    std::cout << "File has " << p_f->size() << " outer blocks\n\n";
    std::vector<Block*>::const_iterator it;
    for (it = p_f->begin(); it != p_f->end(); ++it)
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
    while (true) {
        std::getline(css, line);
        if (!css.eof()) {
            stream << indent << " content:     " << line << "\n";
        } else {
            break;
        }
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
    stream << indent << " start:       " << b.Start()             << "\n";
    stream << indent << " end:         " << b.End()               << "\n";
    stream << indent << " depth:       " << b.Depth()             << "\n";
    stream << indent << " token:       " << b.TokenStr()          << "\n";
    stream << indent << " expression:  " << b.ExpressionStr()     << "\n";
    stream << indent << " expr token#: " << b.Expression().size() << "\n";
    stream << indent << " condbl type: " << b.CondBlockType()     << "\n";

    std::string header = b.Header();
    std::string footer = b.Footer();
    chomp(header);
    chomp(footer);
    stream << indent << " header:      " << header                << "\n";
    stream << indent << " footer:      " << footer                << "\n";

    stream << indent <<" inner blocks: " << b.size() << "\n";
    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream >> **it;

    stream << indent
           << "=[" << b.Id() << "]============   END  CONDITIONAL BLOCK  "
           << "============[" << b.Id() << "]=" << std::endl;
    return stream;
}


// helpers

void chomp (std::string &s)
{
    size_t nlpos = s.find("\n");
    while (nlpos != std::string::npos) {
        s.replace(nlpos, 1, "");
        nlpos = s.find("\n", nlpos + 1);
    }
}

std::string Indent(int depth)
{
    std::stringstream ss;
    ss << "|";
    for (int i=0; i < depth; i++)
        ss << "    |";
    return ss.str();
}


bool ziztest(std::string file, Mode mode)
{
    std::cerr << "Testing " << file << std::endl;
    Ziz::Parser parser;
    Ziz::File* p_zfile;
    try {
        p_zfile = parser.Parse(file);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
        return false;
    } catch(...) {
        std::cerr << "caught exception" << std::endl;
        return false;
    }
    if (mode == Short) {
        std::cout + p_zfile;
    } else if (mode == Medium) {
        std::cout << p_zfile;
    } else if (mode == Long) {
        std::cout >> p_zfile;
    } else {
        assert(false); // shall never happen
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0])
	          << " [-s|--short|-l|--long] FILE [FILES]" << std::endl;
        return 0;
    }

    Mode mode = Medium;
    int fileArg = 1;
    std::string arg(argv[1]);
    if (arg.compare("-l") == 0 || arg.compare("--long") == 0) {
	fileArg++;
	mode = Long;
    } else if (arg.compare("-s") == 0 || arg.compare("--short") == 0) {
	fileArg++;
	mode = Short;
    }

    int fail = 0;
    for (int i = fileArg; i < argc; i++) {
        std::string file(argv[i]);
        if (ziztest(file, mode)) {
            std::cerr << file << " ok" << std::endl;
        } else {
            std::cerr << file << " failed" << std::endl;
            fail++;
        }
    }

    if (fail == 0) {
        std::cerr << "All tests passed." << std::endl;
    } else {
        std::cerr << "Some tests failed." << std::endl;
    }
    return fail;
}