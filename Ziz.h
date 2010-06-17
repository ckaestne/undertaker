/*
 * This header defines the public API of libziz.
 */

#ifndef ZIZ_H_
#define ZIZ_H_

#include <stdexcept>
#include <string>
#include <vector>

// we require boost >= 1.40, as 1.38 has an ugly namespace bug when compiled
// with gcc 4.4
#include <boost/wave.hpp>
#include <boost/wave/cpplexer/cpp_lex_token.hpp>    // token class
#include <boost/wave/cpplexer/cpp_lex_iterator.hpp> // lexer class
#include <boost/wave/util/flex_string.hpp>

typedef boost::wave::cpplexer::lex_token<>              token_type;
typedef boost::wave::cpplexer::lex_iterator<token_type> lexer_type;
typedef token_type::string_type                         string_type;
typedef token_type::position_type                       position_type;
typedef boost::wave::util::file_position_type           file_position_type;


namespace Ziz {

typedef enum {
    Code,
    Conditional
} block_type;

typedef enum {
    OuterBlock,
    InnerBlock
} container_type;

class BlockContainer;

class Block {
    protected:
        // only allow in derived classes
        Block(int i, int d, position_type s, BlockContainer* pbc)
            : _id(i), _depth(d), _start(s), _pParent(pbc) {}
        virtual ~Block() {}
    private:
        Block(Block&);                        // disable copy c'tor

    public:
        virtual block_type BlockType()  const = 0;

        int             Id()      const { return _id; }
        int             Depth()   const { return _depth; }
        position_type   Start()   const { return _start; }
        position_type   End()     const { return _end; }
        BlockContainer* Parent()  const { return _pParent; }

        void SetEnd    (const position_type p) { _end   = p; }

    private:
        int             _id;
        int             _depth;
        position_type   _start;
        position_type   _end;
        BlockContainer* _pParent;
};

class BlockContainer : public std::vector<Block*> {
    protected:
        BlockContainer() {}   // only allow in derived classes
    public:
        virtual container_type ContainerType()  const = 0;
};

class CodeBlock : public Block {
    public:
        CodeBlock(int i, int d, position_type s, BlockContainer* pbc)
            : Block(i, d, s, pbc) {}
        virtual ~CodeBlock() {}
    private:
        CodeBlock(CodeBlock&);   // disable copy c'tor

    public:
        virtual block_type BlockType()  const { return Code; }

        std::string     Content() const { return _content.str(); }

        void AppendContent(const std::string &c) { _content << c; }
        void AppendContent(const string_type &c) { _content << c; }

    private:
        std::stringstream       _content;
};

class ConditionalBlock : public Block, public BlockContainer {
    public:
        ConditionalBlock(int i, int d, position_type s, token_type t,
                         BlockContainer* pbc)
            : Block(i, d, s, pbc), _type(t) {}
        virtual ~ConditionalBlock() {}
    private:
        ConditionalBlock(ConditionalBlock&);   // disable copy c'tor

    public:
        virtual block_type     BlockType()     const { return Conditional; }
        virtual container_type ContainerType() const { return InnerBlock; }

        token_type             TokenType()   const { return _type; }
        std::string            TokenStr()    const;
        std::string            Header()      const { return _header.str(); }
        std::string            Footer()      const { return _footer.str(); }
        std::string            Expression()  const { return _expression.str(); }

        void AppendHeader      (const std::string &s) { _header     << s; }
        void AppendHeader      (const string_type &s) { _header     << s; }
        void AppendFooter      (const std::string &s) { _footer     << s; }
        void AppendFooter      (const string_type &s) { _footer     << s; }
        void AppendExpression  (const std::string &s) { _expression << s; }
        void AppendExpression  (const string_type &s) { _expression << s; }

    private:
        token_type              _type;
        std::stringstream       _header;
        std::stringstream       _footer;
        std::stringstream       _expression;    // TODO: shall be formula later

        // TODO previous_else, ...?
};

class File : public BlockContainer {
    public:
        File() : _blocks(0) {}

    public:
        virtual container_type ContainerType() const { return OuterBlock; }

        CodeBlock*        CreateCodeBlock       (int, position_type,
                                                 BlockContainer*);
        ConditionalBlock* CreateConditionalBlock(int, position_type,
                                                 BlockContainer*, lexer_type&);

    private:
        int _blocks;
};


class ZizException : public std::runtime_error {
    public:
        ZizException() : std::runtime_error("unknown ziz error") {}
        ZizException(const char* c) : std::runtime_error(c) {}
};

class Parser {
    public:
        Parser() : _p_curCodeBlock(NULL),
                   _p_curBlockContainer(&_file)  // add outermost blocks to file
            {}

        File Parse(std::string file);

    private:
        Parser(Parser&);   // disable copy c'tor

        void HandleIF    (lexer_type&);
        void HandleIFDEF (lexer_type&);
        void HandleIFNDEF(lexer_type&);
        void HandleELSE  (lexer_type&);
        void HandleELIF  (lexer_type&);
        void HandleENDIF (lexer_type&);
        void HandleToken (lexer_type&);

        void FinishSaveCurrentCodeBlock();
        void FinishSaveCurrentConditionalBlock(lexer_type&);

        // The block structure of the file that Parse() builds.
        File _file;

        // current file position is saved for exception handling
        file_position_type              _curPos, _prevPos;

        // Stack of open conditional blocks. Pushed to when entering #ifdef
        // (and similar) blocks, popped from when leaving them.
        std::stack<ConditionalBlock*>   _condBlockStack;

        // Current CodeBlock as it is built
        CodeBlock*                      _p_curCodeBlock;
        // Current BlockContainer: where new inner blocks get added to
        BlockContainer*                 _p_curBlockContainer;
};

} // namespace Ziz

std::ostream & operator+ (std::ostream &stream, Ziz::File             const &);
std::ostream & operator+ (std::ostream &stream, Ziz::Block            const &);
std::ostream & operator+ (std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator<<(std::ostream &stream, Ziz::File             const &);
std::ostream & operator<<(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator<<(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator<<(std::ostream &stream, Ziz::ConditionalBlock const &);

std::ostream & operator>>(std::ostream &stream, Ziz::File             const &);
std::ostream & operator>>(std::ostream &stream, Ziz::Block            const &);
std::ostream & operator>>(std::ostream &stream, Ziz::CodeBlock        const &);
std::ostream & operator>>(std::ostream &stream, Ziz::ConditionalBlock const &);

#endif /* ZIZ_H_ */
