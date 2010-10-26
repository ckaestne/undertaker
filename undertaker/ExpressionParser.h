// -*- mode: c++
#ifndef expressionparser_h__
#define expressionparser_h__

#include <string>
#include <list>
#include <deque>
#include <map>
#include <bdd.h>

typedef enum {ident, conj, disj, neg, lparen, rparen, comp, endmarker} SymbolType;

class ExpressionParser {
public:

    struct Symbol {
    Symbol(SymbolType s, std::string t)
      : Sym(s), Token(t) {}
    SymbolType Sym;
    std::string Token;
    };

    struct SymbolList : public std::deque<Symbol> {
    void push_back(std::string item, bool kconfig_mode=false);
    void push_back(Symbol s) { std::deque<Symbol>::push_back(s); }
    };

    ExpressionParser(std::string expression, bool debug=false, bool kconfig_mode=false);

    bdd op(bdd X);
    bdd term(bdd X);
    bdd parse();
    bdd expression();

    typedef std::list<std::string> VariableList;
    VariableList found_variables() { return found_variables_; }

private:

    bool accept(SymbolType s);
    bool expect(SymbolType s);
    bdd faklist();
    bdd term();
    void find_variables (std::string &expression, bool kconfig_mode);
    void getsym();

    Symbol &sym() { return *currsym_; }
    Symbol &prevsym() { return *(currsym_-1); }
    Symbol &nextsym() { return *(currsym_+1); }
    std::string &token() { return sym().Token; }
    std::string &prevtoken() { return prevsym().Token; }
    std::string &nexttoken() { return nextsym().Token; }

    SymbolList symbols_;
    SymbolList::iterator currsym_;

    std::string &expression_;
    bool iselse_;
    bool debug_;
    VariableList found_variables_;
};

bool operator==(const ExpressionParser::Symbol &s, const SymbolType &t);

#endif
