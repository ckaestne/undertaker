// -*- mode: c++
#ifndef KconfigBdd_h__
#define KconfigBdd_h__

#include <fstream>

#include "ExpressionParser.h"
#include "RsfBlocks.h"
#include "VariableToBddMap.h"

class KconfigBdd {
public:
    KconfigBdd(std::ifstream &in, bool debug=false);

    bdd kconfigDependencies(std::string);

    RsfBlocks *getItems() { return &items_;}
    RsfBlocks *getDepends() { return &depends_;}

    typedef ExpressionParser::VariableList VariableList;
protected:
    bdd X;
    std::istream &in_;
    RsfBlocks items_;
    RsfBlocks depends_;
    bool debug_;

    std::string KconfigRsfFile_;
    VariableToBddMap *m;
};

#endif
