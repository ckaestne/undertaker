/*
 *   libziz - parse c preprocessor files
 *
 * Copyright (C) 2010 Frank Blendinger <fb@intoxicatedmind.net>
 * Copyright (C) 2010 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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
#include <boost/regex.hpp>

using namespace Ziz;

static bool print_include = false;
static bool remove_non_CONFIG_blocks = false;


static Ziz::Defines defines;

// search defines in global defines mal for block b
std::list<std::string> search_defs(Block const &b) {
    std::list<std::string> found;

    for (auto & define : defines) {
        const std::string &s = define.first;
        const std::list<Define*> &list = define.second;
        if (list.empty())
            continue;

        for (const auto & elem : list) {
            if (&b == (const Block *) elem->Parent())
                found.push_back(s);
        }
    }
    return found;
}

// output operators

// + short output (--short mode)
std::ostream & operator+(std::ostream &stream, File const * p_f) {
    assert(p_f != nullptr);
    defines = p_f->_defines_map;
    for (const auto &block : *p_f)
        stream + *block;
    return stream;
}

std::ostream & operator+(std::ostream &stream, Block const &b) {
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

std::ostream & operator+(std::ostream &stream, ConditionalBlock const &b) {
    std::list<std::string> found;

    stream << "START BLOCK " << b.Id() << " [T=" << b.TokenStr() << "] ";

    stream << "[E=" << b.ExpressionStr() << "] ";

    std::string header = b.Header();
    chomp(header);
    stream << "[H=" << header << "] ";

    found = search_defs(b);
    if (!found.empty()) {
        stream << "[D=";
        for (auto & elem : found)
            stream << elem << " ";
        stream << "] ";
    }

    std::string footer = b.Footer();
    chomp(footer);
    stream << "[F=" << footer << "] [P=";

    BlockContainer* p_parent = b.Parent();
    assert(p_parent != nullptr);
    if (p_parent->ContainerType() == OuterBlock) {
        stream << "<none>";
    } else if (p_parent->ContainerType() == InnerBlock) {
        stream << dynamic_cast<ConditionalBlock*>(p_parent)->Id();
    } else {
        assert(false);      // this may not happen
    }

    stream << "] [PS=";
    ConditionalBlock* p_prevSibling = b.PrevSibling();
    if (p_prevSibling == nullptr) {
        stream << "<none>";
    } else {
        stream << p_prevSibling->Id();
    }

    stream << "]\n";

    for (const auto &block : b)
        stream + *block;

    stream << "END BLOCK " << b.Id() << "\n";
    return stream;
}

// + convert output (--convert mode)

static bool block_has_CONFIG(ConditionalBlock const &b) {
    std::stringstream ss;
    ss << b.Header();
    std::string header = ss.str();
    return header.find("CONFIG_") != std::string::npos;
}

static int subtree_CONFIG_blocks(BlockContainer const &parent, Block const &b) {
    int blocks = 0;
    if (b.BlockType() == Code) {
        return 1;
    } else if (b.BlockType() == Conditional) {
        ConditionalBlock const &cb = dynamic_cast<ConditionalBlock const &>(b);
        if (block_has_CONFIG(cb))
            return 1;

        bool skip = true;

        ConditionalBlock *ptr = (ConditionalBlock *) &b;

        while(!(ptr->CondBlockType() == Ziz::If
                || ptr->CondBlockType() == Ziz::Ifdef
                || ptr->CondBlockType() == Ziz::Ifndef))
            ptr = ptr->PrevSibling();

        for (const auto &block : parent) {
            if (block == ptr)
                skip = false;
            if (skip == true)
                continue;

            if (block->BlockType() != Conditional)
                continue;

            ConditionalBlock const &inner_block = dynamic_cast<ConditionalBlock const &>(*block);

            // COPIED from undertaker/ConditionalBlock.cpp
            if (block != ptr) {
                if (inner_block.CondBlockType() == Ziz::If
                    || inner_block.CondBlockType() == Ziz::Ifdef
                    || inner_block.CondBlockType() == Ziz::Ifndef)
                    break;

            }

            if (block_has_CONFIG(inner_block))
                return 1;

            for (const auto & elem : inner_block) {
                if (elem->BlockType() != Conditional)
                    continue;
                blocks += subtree_CONFIG_blocks(inner_block, *elem);
                if (blocks > 0)
                    return blocks;
            }
        }

    } else if (b.BlockType() == DefineBlock) {
        return 1;
    } else {
        assert(false);      // this may not happen
    }
    return blocks;
}

std::vector<int> code_lines_stack;


std::ostream & operator*(std::ostream &stream, File const * p_f) {
    code_lines_stack.push_back(0);

    assert(p_f != nullptr);
    defines = p_f->_defines_map;
    for (const auto &elem : *p_f) {
        if (remove_non_CONFIG_blocks && subtree_CONFIG_blocks(*p_f, *elem) == 0)
            continue;
        stream * *elem;
    }
    stream << "B00 " << code_lines_stack.back() << std::endl;
    code_lines_stack.pop_back();
    assert (code_lines_stack.size() == 0);

    return stream;
}

std::ostream & operator*(std::ostream &stream, Block const &b) {
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

std::ostream & operator*(std::ostream &stream, ConditionalBlock const &b) {
    code_lines_stack.push_back(0);
    stream << b.Header() << std::endl;

    for (const auto &block : b) {
        if (remove_non_CONFIG_blocks && subtree_CONFIG_blocks(b, *block) == 0)
            continue;
        stream * *block;
    }
    stream << "B" << b.Id() << " " << code_lines_stack.back() << std::endl;

    code_lines_stack.pop_back();

    stream << b.Footer() << std::endl;

    return stream;
}

std::ostream & operator*(std::ostream &stream, CodeBlock const &b) {
    std::stringstream content (b.Content());
    std::string line;

    boost::regex cpp_regexp(print_include ? "^[ \t]*#[ \t]*(define|undef|include)"
                                          : "^[ \t]*#[ \t]*(define|undef)");

    int lines = 0;
    while (getline (content, line)) {
        lines ++;
        if (boost::regex_search(line, cpp_regexp)) {
            stream << line << std::endl;
        }
    }

    code_lines_stack.back() += lines;

    return stream;
}

// << normal output (default mode)
std::ostream & operator<<(std::ostream &stream, File const * p_f) {
    assert(p_f != nullptr);
    for (const auto & elem : *p_f)
        stream << *elem;
    return stream;
}

std::ostream & operator<<(std::ostream &stream, Block const &b) {
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

std::ostream & operator<<(std::ostream &stream, CodeBlock const &b) {
    std::list<std::string> found;

    stream << "START CODE BLOCK " << b.Id() << "\n";
    stream << b.Content();
    found = search_defs(b);
    if (!found.empty()) {
        stream << "[#defines: ";
        for (auto & elem : found)
            stream << elem << " ";
        stream << "] ";
    }
    stream << "END CODE BLOCK " << b.Id() << "\n";
    return stream;
}

std::ostream & operator<<(std::ostream &stream, ConditionalBlock const &b) {
    stream << "START CONDITIONAL BLOCK " << b.Id() << "\n";
    stream << "token="      << b.TokenStr()    << "\n";
    stream << "header="     << b.Header()      << "\n";
    //stream << "expression=" << b.Expression()  << "\n"; // FIXME
    stream << "footer="     << b.Footer()      << "\n";

    for (const auto & elem : b)
        stream << *elem;

    stream << "END CONDITIONAL BLOCK " << b.Id() << "\n";
    return stream;
}

// >> verbose output (--long mode)
std::ostream & operator>>(std::ostream &stream, File const * p_f) {
    assert(p_f != nullptr);
    std::cout << "File has " << p_f->size() << " outer blocks\n\n";
    for (const auto & elem : *p_f)
        stream >> *elem;
    return stream;
}

std::ostream & operator>>(std::ostream &stream, Block const &b) {
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

std::ostream & operator>>(std::ostream &stream, CodeBlock const &b) {
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

std::ostream & operator>>(std::ostream &stream, ConditionalBlock const &b) {
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
    for (const auto & elem : b)
        stream >> *elem;

    stream << indent
           << "=[" << b.Id() << "]============   END  CONDITIONAL BLOCK  "
           << "============[" << b.Id() << "]=" << std::endl;
    return stream;
}


// helpers

void chomp (std::string &s) {
    size_t nlpos = s.find("\n");
    while (nlpos != std::string::npos) {
        s.replace(nlpos, 1, "");
        nlpos = s.find("\n", nlpos + 1);
    }
}

std::string Indent(int depth) {
    std::stringstream ss;
    ss << "|";
    for (int i=0; i < depth; i++)
        ss << "    |";
    return ss.str();
}


bool ziztest(std::string file, Mode mode) {
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

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << std::string(argv[0])
                  << " [-s|--short|-l|--long|-c|--convert|-cI|--convert-include|-cC|--convert-configurable] FILE [FILES]" << std::endl;
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
    } else if (arg.compare("-cI") == 0 || arg.compare("--convert-include") == 0) {
        fileArg++;
        mode = Convert;
        print_include = true;
    } else if (arg.compare("-cC") == 0 || arg.compare("--convert-configurable") == 0) {
        fileArg++;
        mode = Convert;
        remove_non_CONFIG_blocks = true;
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
