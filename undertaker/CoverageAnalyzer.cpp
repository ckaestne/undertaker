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

std::string CoverageAnalyzer::baseFileExpression(const ConfigurationModel *model) {
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

    return formula.join(" && ");
}

std::list<SatChecker::AssignmentMap> SimpleCoverageAnalyzer::blockCoverage(ConfigurationModel *model) {
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;
    std::set<SatChecker::AssignmentMap> found_solutions;

    const std::string base_formula = baseFileExpression(model);

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
                    const std::string &name = (*it).first;
                    const bool enabled = (*it).second;
                    static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);

                    if (boost::regex_match(name, block_regexp)) {
                        // if a block is enabled, and not already in
                        // the block set, we enable it with this
                        // configuration and get a new solution
                        if (enabled && blocks_set.find(name) == blocks_set.end()) {
                            blocks_set.insert(name);
                            new_solution = true;
                        }
                        // No blocks in the assignment maps
                        continue;
                    }

                    /* If no model is given or the symbol is in the
                       model space we can push the assignment to the
                       current solution.
                    */
                    if (!model || model->inConfigurationSpace(name)) {
                        current_solution.insert(std::make_pair<std::string,bool>(name, enabled));
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

std::list<SatChecker::AssignmentMap> MinimizeCoverageAnalyzer::blockCoverage(ConfigurationModel *model) {
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;

    const std::string base_formula = baseFileExpression(model);

    try {
        while (blocks_set.size() != file->size()) {
            // blocks enabled with this configuration
            UniqueStringJoiner configuration;

            for(CppFile::iterator i  = file->begin(); i != file->end(); ++i) {
                std::string block_name = (*i)->getName();
                // Was already enabled in an other configuration
                if (blocks_set.count(block_name) > 0) continue;

                // We check here if the selected block is surely in
                // conflict with another block already in the current
                // configuration.
                // e.g We have the if clause already in the set, then
                // the else clause will surely not be in the
                // configuration
                {
                    const ConditionalBlock *block = *i;
                    bool conflicting = false;
                    while (block && block != file->topBlock()) {
                        if (configuration.count(block->getName()) > 0) {
                            conflicting = true;
                            break;
                        }
                        if (block->isIfBlock()) break;
                        block = block->getPrev();
                    }
                    if (conflicting) continue;
                }
                std::string conf_exp;
                if (configuration.size() > 0)
                    conf_exp = " && " + configuration.join(" && ");

                SatChecker check(block_name + conf_exp + " && " + base_formula);

                if (!check()) {
                    // Block couldn't be enabled
                    if (configuration.size() == 0) {
                        // dead block; just ignore it
                        blocks_set.insert(block_name);
                    }
                    // Block cannot be enabled with current
                    // <configuration> block set
                    continue;
                } else {
                    // Block will be enabled with this configuration
                    blocks_set.insert(block_name);
                    configuration.push_back(block_name);
                }
            }

            // This formula must be true, since it was checked in the
            // already
            if (configuration.size() == 0) continue;
            SatChecker assignment_sat(configuration.join(" && ") + " && " + base_formula);
            assert(assignment_sat());

            const SatChecker::AssignmentMap &assignments = assignment_sat.getAssignment();
            ret.push_back(assignments);
        }
    } catch (SatCheckerError &e) {
        std::cerr << "Couldn't process " << file->getFilename() << ": "
                  << e.what() << std::endl;
    }
    return ret;
}
