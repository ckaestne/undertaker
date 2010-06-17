// -*- mode: c++ -*-
#ifndef bdd_loader_h__
#define bdd_loader_h__

#include <string>
#include "bdd.h"

class BddLoader {
public:
    BddLoader(std::string path);
    int makeBdd(bdd &b);

private:
    static const char *CACHE_DIR;
    std::string _path;
};

#endif
