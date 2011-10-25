/*
 *   (feline) predator - normalizes cpp statements in a source file
 *
 * Copyright (C) 2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#include <Puma/CParser.h>
#include <Puma/CProject.h>
#include <Puma/CTranslationUnit.h>
#include <Puma/PreTree.h>
#include <Puma/ManipCommander.h>
#include <Puma/PreParser.h>

#include "PredatorVisitor.h"

#include <boost/thread.hpp>

Puma::CProject *project;
Puma::ErrorStream err;

/// \brief cuts out all \#include statements
Puma::Unit *cut_includes(Puma::Unit *unit) {
    Puma::ManipCommander mc;
    Puma::Token *s, *e;

restart:
    for (s = unit->first(); s != unit->last(); s = unit->next(s)) {
        switch(s->type()) {
        case TOK_PRE_ASSERT:
        case TOK_PRE_ERROR:
        case TOK_PRE_INCLUDE:
        case TOK_PRE_INCLUDE_NEXT:
        case TOK_PRE_WARNING:
            e = s;
            do {
                e = unit->next(e);
            } while (e->text()[0] != '\n');
            mc.kill(s, e);
            mc.commit();
            goto restart;
        }
    }
    return unit;
}

void usage(const char *msg, bool fail=true) {
    std::cout << "predator <file>" << std::endl;
    std::cout << msg << std::endl;
    if (fail)
        exit(EXIT_FAILURE);
}

void normalize_file(const char *f) {
    // See manual or Config.cc in the Puma sources for Puma command line arguments
    Puma::CParser parser;

    Puma::Unit *unit = project->scanFile(f);
    if (!unit) {
        std::cout << "Failed to parse: " << f << std::endl;
        exit(EXIT_FAILURE);
    }

    unit = cut_includes(unit);
    Puma::CTranslationUnit *file = parser.parse(*unit, *project, 2); // cpp tree only!

    Puma::PreTree *ptree = file->cpp_tree();
    assert(ptree);

    PredatorVisitor predator(err);
    ptree->accept(predator);
    delete file;
}

int main(int argc, char **argv) {
    const char *filename = argv[argc-1];

    project = new Puma::CProject(err, argc, argv);
    if (argc <2)
        usage("Need at least two arguments");

    boost::thread t(normalize_file, filename);

    if (!t.timed_join(boost::posix_time::seconds(300))) {
        std::cerr << "E: timeout passed while processing " << filename
                  << std::endl;
        delete project;
        return 1;
    }

    delete project;
    return 0;
}
