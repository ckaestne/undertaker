/*
 *   undertaker - analyze preprocessor blocks in code
 *
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

#ifndef _COVERAGEANALYZER_H_
#define _COVERAGEANALYZER_H_

#include "ConditionalBlock.h"
#include "ConfigurationModel.h"
#include "SatChecker.h"

class CoverageAnalyzer {
 public:
    CoverageAnalyzer(CppFile *);

    std::list<SatChecker::AssignmentMap> blockCoverage(ConfigurationModel *);

 private:
    CppFile * file;
};

#endif /* _COVERAGEANALYZER_H_ */
