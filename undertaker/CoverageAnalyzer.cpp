/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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

#include "CoverageAnalyzer.h"
#include "StringJoiner.h"
#include "ConditionalBlock.h"
#include "ConfigurationModel.h"
#include "exceptions/CNFBuilderError.h"
#include "Logging.h"


/************************************************************************/
/* CoverageAnalyzer                                                     */
/************************************************************************/

std::string CoverageAnalyzer::baseFileExpression(const ConfigurationModel *model) {
    StringJoiner formula;
    std::string code_formula = file->topBlock()->getCodeConstraints();
    formula.push_back(code_formula);

    if (model) {
        std::string kconfig_formula;
        model->doIntersect(code_formula, file->getChecker(), missingSet, kconfig_formula);
        formula.push_back(kconfig_formula);
        // only add missing items if we can assume the model is complete
        if (model->isComplete()) {
            for (const std::string &str : missingSet)
                formula.push_back("!" + str);
        }
        // add ALWAYS_ON items to the formula
        const std::string magic1("ALWAYS_ON");
        const StringList *always_on = model->getMetaValue(magic1);
        if (always_on) {
            logger << info << always_on->size() << " Items have been forcefully set" << std::endl;
            for (const std::string &str : *always_on)
                formula.push_back(str);
        }
        // add ALWAYS_OFF items to the formula
        const std::string magic2("ALWAYS_OFF");
        const StringList *always_off = model->getMetaValue(magic2);
        if (always_off) {
            logger << info << always_off->size() << " Items have been forcefully unset"
                   << std::endl;
            for (const std::string &str : *always_off)
                formula.push_back("!" + str);
        }
    }
    logger << debug << "baseFileExpression: " << formula.join("\n&& ") << std::endl;
    return formula.join(" && ");
}

/************************************************************************/
/* SimpleCoverageAnalyzer                                               */
/************************************************************************/

std::list<SatChecker::AssignmentMap> SimpleCoverageAnalyzer::blockCoverage(ConfigurationModel *model) {
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;
    std::set<SatChecker::AssignmentMap> found_solutions;

    const std::string base_formula = baseFileExpression(model);

    try {
        BaseExpressionSatChecker sc(base_formula);

        for (const auto &block : *file) {      // ConditionalBlock *
            SatChecker::AssignmentMap current_solution;

            if (blocks_set.find(block->getName()) == blocks_set.end()) {
                /* does this block contribute to the set of configurations? */
                bool new_solution = false;

                // unsolvable, i.e. we have found some defect!
                if (!sc( { block->getName() } ))
                    continue;

                static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
                for (const auto &assignment : sc.getAssignment()) { // pair<string, bool>
                    const std::string &name = assignment.first;
                    const bool enabled = assignment.second;

                    if (boost::regex_match(name, block_regexp)) {
                        // if a block is enabled, and not already in the block set, we enable it
                        // with this configuration and get a new solution
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
                    if (!model || model->inConfigurationSpace(name))
                        current_solution.emplace(name, enabled);
                }

                if (found_solutions.find(current_solution) == found_solutions.end()) {
                    found_solutions.insert(current_solution);

                    if (new_solution)
                        ret.push_back(sc.getAssignment());
                }
            }
        }
    } catch (CNFBuilderError &e) {
        logger << error << "Couldn't process " << file->getFilename()
            << ": " << e.what() << std::endl;
    } catch (std::bad_alloc &e) {
        logger << error << "Couldn't process " << file->getFilename()
            << ": Out of Memory." << std::endl;
    }
    return ret;
}

/************************************************************************/
/* MinimizeCoverageAnalyzer                                             */
/************************************************************************/

std::list<SatChecker::AssignmentMap> MinimizeCoverageAnalyzer::blockCoverage(ConfigurationModel *model) {
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;

    try {
        std::set<std::string> configuration;

        // Initial Phase, we start the SAT Solver for the whole
        // formula. Because it tries so maximize the enabled
        // variables we get a configuration for many of the blocks as
        // in the simple algorithm. For the all blocks not enabled
        // there we do the minimizer algorithm
        const std::string base_formula = baseFileExpression(model);
        BaseExpressionSatChecker sc(base_formula);

        if(sc(configuration)) { // Configuration is an empty list here
            static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);
            for (const auto &assignment : sc.getAssignment()) {  // pair<string, bool>
                if (assignment.second == false) continue; // Not enabled
                const std::string &block_name = assignment.first;
                if (boost::regex_match(block_name, block_regexp)) {
                    configuration.insert(block_name);
                    blocks_set.insert(block_name);
                }
            }
            goto dump_configuration;
        }

        // For the first round, configuration size will be non-zero at this point
        while (blocks_set.size() < file->size()) {
            for(const auto &block : *file) {  // ConditionalBlock *
                std::string block_name = block->getName();

                // Was already enabled in an other configuration
                if (blocks_set.count(block_name) > 0) continue;

                // We check here if the selected block is surely in
                // conflict with another block already in the current
                // configuration.
                // e.g We have the if clause already in the set, then
                // the else clause will surely not be in the
                // configuration
                {
                    ConditionalBlock *block_it = block;
                    bool conflicting = false;
                    while (block_it && block_it != file->topBlock()) {
                        if (configuration.count(block_it->getName()) > 0) {
                            conflicting = true;
                            break;
                        }
                        if (block_it->isIfBlock()) break;
                        block_it = const_cast<ConditionalBlock *>(block_it->getPrev());
                    }
                    if (conflicting) continue;
                }

                configuration.insert(block->getName());

                if (!sc(configuration)) {
                    // Block couldn't be enabled
                    if (configuration.size() == 1) {
                        // dead block; just ignore it
                        blocks_set.insert(block->getName());
                        configuration.clear();
                    }
                    configuration.erase(block->getName());
                    // Block cannot be enabled with current
                    // <configuration> block set
                    continue;
                } else {
                    // Block will be enabled with this configuration
                    blocks_set.insert(block_name);
                }
            }
        dump_configuration:
            if (configuration.size() == 0) continue;

            assert(sc(configuration));
            ret.push_back(sc.getAssignment());

            // We have added an assignment, so we can clear the
            // configuration-set for the next configuration
            configuration.clear();
            assert(configuration.size() == 0);
        }
    } catch (CNFBuilderError &e) {
        logger << error << "Couldn't process " << file->getFilename()
            << ": " << e.what() << std::endl;
    } catch (std::bad_alloc &) {
        logger << error << "Couldn't process " << file->getFilename()
            << ": Out of Memory." << std::endl;
    }
    return ret;
}
