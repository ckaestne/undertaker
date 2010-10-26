#include "ExpressionParser.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <cstdlib>
#include <stdexcept>
#include <ostream>

#include "VariableToBddMap.h"

void ExpressionParser::SymbolList::push_back(std::string item, bool kconfig_mode) {

    if (item == ".")
    this->push_back(Symbol(endmarker, item));
    else if (item == "(")
    this->push_back(Symbol(lparen, item));
    else if (item == ")" )
    this->push_back(Symbol(rparen, item));
    else if (item == "||")
    this->push_back(Symbol(disj, item));
    else if (item == "&&")
    this->push_back(Symbol(conj, item));
    else if (item == "!")
    this->push_back(Symbol(neg, item));
    else if (item == "<")
    this->push_back(Symbol(conj, item));
    else if (item == ">")
    this->push_back(Symbol(comp, item));
    else if (item == "%")
    this->push_back(Symbol(comp, item));
    else if (item == "<=")
    this->push_back(Symbol(comp, item));
    else if (item == ">=")
    this->push_back(Symbol(comp, item));
    else if (item == "==")
    this->push_back(Symbol(comp, item));
    else if (item == "!=")
    this->push_back(Symbol(comp, item));
    else if (kconfig_mode)
    this->push_back(Symbol(ident, "CONFIG_" + item));
    else
    this->push_back(Symbol(ident, item));
}

ExpressionParser::ExpressionParser(std::string expression, bool debug,
                   bool kconfig_mode)
    : expression_(expression), iselse_(false), debug_(debug) {

    std::stringstream ss(expression);
    std::string item;

    find_variables(expression, kconfig_mode);

    while (ss >> item) {
    if (item == "]")
        continue;

    if (item == "of:")
        continue;

    if (item == "[else") {
        iselse_ = true;
        continue;
    }

    symbols_.push_back(item, kconfig_mode);
    }
    symbols_.push_back(".");
    currsym_ = symbols_.begin();
}

std::string decode(SymbolType s) {
    std::stringstream ss;

    switch(s) {
    case ident: ss << "identifier"; break;
    case neg: ss << "neg"; break;
    case conj:
    case disj: ss << "conj/disj"; break;
    case lparen: ss << "("; break;
    case rparen: ss << ")"; break;
    case comp: ss <<  "comp" ; break;
    case endmarker: ss << "end of expr"; break;
    }
    return ss.str();
}

bool ExpressionParser::accept(SymbolType s) {
    if (currsym_ == symbols_.end())
    return false;

    if (sym() == s) {
    if (debug_)
        std::cout << "   -> accepting: " << decode(s) << std::endl;
    getsym();
    return true;
    }
    return false;
}


bool ExpressionParser::expect(SymbolType s) {

    if (accept(s))
    return 1;

    std::stringstream ss;
    ss << "failed to parse: " << expression_ << std::endl;
    ss << "Unexpected token: " << token();
    ss << ", type:" << decode(sym().Sym);
    ss << ", expected: " << decode(s);
    ss << std::endl;

    throw std::runtime_error(ss.str());
}

bdd ExpressionParser::op(bdd X) {
    if (accept(conj))
        return X & expression();

    if (accept(disj))
        return X | expression();

    if (accept (comp)) {
        std::string c = prevtoken();
        std::stringstream ss;
        ss << "<term: '" << c << "'>";
        if (accept(lparen))
        term();
        accept(ident);
        return VariableToBddMap::getInstance()->add_new_variable(ss.str());
    }
    return X;
}

bdd ExpressionParser::term() {
    bdd X = expression();

    if (accept(rparen)) {
    if (accept(endmarker))
        return X;

    if (accept(conj))
        return X & expression();

    if (accept(disj))
        return X | expression();

    if (accept (comp)) {
        std::string c = prevtoken();
        std::stringstream ss;
        ss << "<term: '" << c << "'>";
        if (accept(lparen))
        term();
        return VariableToBddMap::getInstance()->add_new_variable(ss.str());
    }
    }
    return X;
}

bdd ExpressionParser::expression() {
    bool doneg = false;

    if (iselse_){
    iselse_ = false;
    return bdd_not(expression());
    }

    bdd X;

    while (accept(neg)) {
    doneg = !doneg;
    }

    if (accept(ident)) {
        X = VariableToBddMap::getInstance()->get_bdd(prevtoken());
    if(doneg)
        X = bdd_not(X);
    X = op(X);
    }


    if (accept (lparen)) {
    if (doneg)
        return bdd_not(term());
    else
        return term();
    }

    X = op(X);

    accept(endmarker);
    return X;
}

bdd ExpressionParser::parse() {

    try {
    return expression();
    } catch (std::exception &e) {
    std::clog << "parse error: " << e.what() << std::endl;
    std::clog << "failed to parse '" << expression_ << "'\n";
    exit(EXIT_FAILURE);
    }
}


void ExpressionParser::getsym () {
    currsym_++;
}

void ExpressionParser::find_variables (std::string &expression, bool kconfig_mode) {
    std::stringstream ss(expression);
    std::string item;

    while(ss >> item) {

    if (item == "[else")
        continue;

    if (item == "of:")
        continue;

    if (item == "]")
        continue;

    if (item == "(" || item == ")" || item == "!" || item == "%" ||
        item == "&&" || item == "||" || item == "==" || item == "!=" ||
        item == "<" || item == ">" || item == ".")
        continue; // ignore specials

    if(kconfig_mode)
        item = "CONFIG_" + item;

    VariableToBddMap::getInstance()->add_new_variable(item);
    }
    return;
}


bool operator==(const ExpressionParser::Symbol &s, const SymbolType &t) {
    return s.Sym == t;
}
