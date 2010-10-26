#ifndef COMPILATION_UNIT_INFO_H__
#define COMPILATION_UNIT_INFO_H__

#include <string>
#include "RsfBlocks.h"

class CompilationUnitInfo {
public:
    CompilationUnitInfo(std::ifstream &in, std::ostream &log)
    : _in(in),
      expressions_(in, "CondBlockHasExpression", log),
      nested_in_(in, "CondBlockNestedIn", log),
      YoungerSibl_(in, "YoungerSilb", log),
      OlderSibl_(in, "OlderSilb", log)
    {}

    const RsfBlocks &expressions() { return expressions_; }
    const RsfBlocks &nested_in() { return nested_in_; }
    const RsfBlocks &YoungerSibl() { return YoungerSibl_; }
    const RsfBlocks &OlderSibl() { return OlderSibl_; }

protected:
    std::ifstream &_in;
    RsfBlocks expressions_;
    RsfBlocks nested_in_;
    RsfBlocks YoungerSibl_;
    RsfBlocks OlderSibl_;
};

#endif
