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


#include "Zizler.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace Ziz;


static Ziz::Defines defines;

// search defines in global defines mal for block b
std::list<std::string> search_defs(Block const &b) {
    std::list<std::string> found;

    for (Defines::iterator i = defines.begin(); i != defines.end(); i++) {
        const std::string &s = (*i).first;
        const std::list<Define*> &list = (*i).second;
        if (list.empty())
            continue;

        for (std::list<Define*>::const_iterator d = list.begin(); d != list.end(); d++) {
            if (&b == (const Block *) (*d)->Parent())
                found.push_back(s);
        }
    }
    return found;
}

// output operators

// + short output (--short mode)
std::ostream & operator+(std::ostream &stream, File const * p_f)
{
    std::vector<Block*>::const_iterator it;

    assert(p_f != NULL);
    defines = p_f->_defines_map;
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
    } else if (b.BlockType() == DefineBlock) {
        //        stream << dynamic_cast<DefineBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator+(std::ostream &stream, ConditionalBlock const &b)
{
    std::list<std::string> found;

    stream << "START BLOCK " << b.Id() << " [T=" << b.TokenStr() << "] ";

    stream << "[E=" << b.ExpressionStr() << "] ";

    std::string header = b.Header();
    chomp(header);
    stream << "[H=" << header << "] ";

    found = search_defs(b);
    if (!found.empty()) {
        stream << "[D=";
        for (std::list<std::string>::iterator i = found.begin(); i!=found.end(); i++)
            stream << (*i) << " ";
        stream << "] ";
    }

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

// + convert output (--convert mode)
std::ostream & operator*(std::ostream &stream, File const * p_f)
{
    std::vector<Block*>::const_iterator it;

    assert(p_f != NULL);
    defines = p_f->_defines_map;
    for (it = p_f->begin(); it != p_f->end(); ++it)
        stream * **it;
    stream << std::endl;
    return stream;
}

std::ostream & operator*(std::ostream &stream, Block const &b)
{
    if (b.BlockType() == Code) {
        stream * dynamic_cast<CodeBlock const &>(b);
    } else if (b.BlockType() == Conditional) {
        stream * dynamic_cast<ConditionalBlock const &>(b);
    } else if (b.BlockType() == DefineBlock) {
        //        stream << dynamic_cast<DefineBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator*(std::ostream &stream, ConditionalBlock const &b)
{
    std::list<std::string> found;

    stream << b.Header();
    stream << "B" << b.Id() << std::endl;
    std::vector<Block*>::const_iterator it;
    for (it = b.begin(); it != b.end(); ++it)
        stream * **it;
    stream << b.Footer() << std::endl;

    return stream;
}

std::ostream & operator*(std::ostream &stream, CodeBlock const &b)
{
    std::list<std::string> found;
    std::stringstream content (b.Content());
    std::string line;
    while (getline (content, line)) {
        if (line.find("#define") != std::string::npos
            || line.find("#undef") != std::string::npos)
            stream << line << std::endl;
    }

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
    } else if (b.BlockType() == DefineBlock) {
        //        stream << dynamic_cast<DefineBlock const &>(b);
    } else {
        assert(false);      // this may not happen
    }
    return stream;
}

std::ostream & operator<<(std::ostream &stream, CodeBlock const &b)
{
    std::list<std::string> found;

    stream << "START CODE BLOCK " << b.Id() << "\n";
    stream << b.Content();
    found = search_defs(b);
    if (!found.empty()) {
        stream << "[#defines: ";
        for (std::list<std::string>::iterator i = found.begin(); i!=found.end(); i++)
            stream << (*i) << " ";
        stream << "] ";
    }
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
    } else if (b.BlockType() == DefineBlock) {
        //        stream << dynamic_cast<DefineBlock const &>(b);
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
    if (mode != Convert)
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
    } else if (mode == Convert) {
        std::cout * p_zfile;
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
                  << " [-s|--short|-l|--long|-c|--convert] FILE [FILES]" << std::endl;
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
    } else if (arg.compare("-c") == 0 || arg.compare("--convert") == 0) {
        fileArg++;
        mode = Convert;
    }


    int fail = 0;
    for (int i = fileArg; i < argc; i++) {
        std::string file(argv[i]);
        if (ziztest(file, mode)) {
            if (mode != Convert)
                std::cerr << file << " ok" << std::endl;
        } else {
            std::cerr << file << " failed" << std::endl;
            fail++;
        }
    }

    if (mode != Convert) {
        if (fail == 0) {
            std::cerr << "All tests passed." << std::endl;
        } else {
            std::cerr << "Some tests failed." << std::endl;
        }
    }
    return fail;
}
