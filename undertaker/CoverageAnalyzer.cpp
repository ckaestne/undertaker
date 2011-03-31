/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


#include "StringJoiner.h"
#include "CoverageAnalyzer.h"

/* c'tor */
CoverageAnalyzer::CoverageAnalyzer(CppFile *file) : file(file) {};

std::list<SatChecker::AssignmentMap>
CoverageAnalyzer::blockCoverage(ConfigurationModel *model) {
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;
    std::set<SatChecker::AssignmentMap> found_solutions;

    const StringList *always_on = NULL;

    if (model) {
        const std::string magic("ALWAYS_ON");
        always_on = model->getMetaValue(magic);
        if (always_on) {
            std::cout << "I: " << always_on->size() << " Items have been forcefully set" << std::endl;
        }
    }

    StringJoiner formula;
    std::string code_formula = file->topBlock()->getCodeConstraints();
    formula.push_back(code_formula);
    if (model) {
        std::set<std::string> missingSet;
        std::string kconfig_formula;
        model->doIntersect(code_formula, file->getChecker(), 
                           missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        /* only add missing items if we can assume the model is
           complete */
        if (model && model->isComplete()) {
            for (MissingSet::iterator it = missingSet.begin(); it != missingSet.end(); it++)
                formula.push_back("!" + (*it));
        }
    }
    if (always_on) {
        for (StringList::const_iterator sli = always_on->begin(); sli != always_on->end(); ++sli) {
            formula.push_back(*sli);
        }
    }

    const std::string base_formula = formula.join(" && ");

    try {
        for(CppFile::iterator i  = file->begin(); i != file->end(); ++i) {
            ConditionalBlock *block = *i;

            SatChecker::AssignmentMap current_solution;

            if (blocks_set.find(block->getName()) == blocks_set.end()) {
                /* does the new contributes to the set of configurations? */
                bool new_solution = false;
                SatChecker sc(block->getName() + " && " + base_formula);

                // unsolvable, i.e. we have found some defect!
                if (!sc())
                    continue;

                const SatChecker::AssignmentMap &assignments = sc.getAssignment();
                SatChecker::AssignmentMap::const_iterator it;
                for (it = assignments.begin(); it != assignments.end(); it++) {
                    static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);

                    /* If no model is given or the symbol is in the
                       model space we can push the assignment to the
                       current solution.
                    */
                    if (!model || model->inConfigurationSpace((*it).first)) {
                        current_solution.insert(std::make_pair<std::string,bool>((*it).first, (*it).second));
                        continue;
                    }

                    if ((*it).second && boost::regex_match((*it).first, block_regexp)) {
                        blocks_set.insert((*it).first);
                        new_solution = true;
                    }
                }

                if (found_solutions.find(current_solution) == found_solutions.end()) {
                    found_solutions.insert(current_solution);

                    if (new_solution)
                        ret.push_back(assignments);
                }
            }
        }
    } catch (SatCheckerError &e) {
        std::cerr << "Couldn't process " << file->getFilename() << ": "
                  << e.what() << std::endl;
    }
    return ret;
}
