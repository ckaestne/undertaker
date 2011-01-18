/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


// -*- mode: c++ -*-
#ifndef blockdefect_h__
#define blockdefect_h__

#include "CodeSatStream.h"

/**
 * \brief Base Class of all Kind of Configuration Defects
 *
 * A BlockDefectAnalyzer checks a CodeSatStream for configuration
 * defects. The known configuration defects are listed in the enum
 * DEFECTTYPE. It provides facilities to test a given block number for a
 * defect and report its results.
 */
struct BlockDefectAnalyzer {
    enum DEFECTTYPE { None, Implementation, Configuration, Referential };

    virtual bool isDefect(const ConfigurationModel *model) = 0; //!< checks for a defect
    virtual bool isGlobal() const = 0; //!< checks if the defect applies to all models
    virtual bool needsCrosscheck() const = 0; //!< defect will be present on every model
    virtual void defectIsGlobal();  //!< mark defect als valid on all models
    const std::string defectTypeToString() const; //!< human readable identifier for the defect type
    const std::string getSuffix() const { return std::string(_suffix); }
    std::string getBlockPrecondition(const ConfigurationModel *model) const;

    /**
     * \brief Write out a report to a file.
     *
     * Write out a report to a file. The Filename is calculated based on the
     * defect type.  Examples:
     *
     * \verbatim
     * filename                    |  meaning: dead because...
     * ---------------------------------------------------------------------------------
     * $block.$arch.code.dead     -> only considering the CPP structure and expressions
     * $block.$arch.kconfig.dead  -> additionally considering kconfig constraints
     * $block.$arch.missing.dead  -> additionally setting symbols not found in kconfig
     *                               to false (grounding these variables)
     * $block.globally.dead       -> dead on every checked arch
     * \endverbatim
     */
    virtual bool writeReportToFile() const = 0;
    virtual void markOk(const std::string &arch);
    virtual std::list<std::string> getOKList() { return _OKList; }
    virtual int defectType() const { return _defectType; }
    virtual ~BlockDefectAnalyzer() {}

protected:
    BlockDefectAnalyzer(int defecttype) : _defectType(defecttype), _isGlobal(false), _OKList() {}
    int _defectType;
    bool _isGlobal;
    const char *_block;
    CodeSatStream *_cs;
    const char *_suffix;
    std::list<std::string> _OKList; //!< List of architectures on which this is proved to be OK
};

//! Checks a given block for "un-selectable block" defects.
class DeadBlockDefect : public BlockDefectAnalyzer {
public:
    //! c'tor for Dead Block Defect Analysis
    DeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const ConfigurationModel *model); //!< checks for a defect
    virtual bool isGlobal() const; //!< checks if the defect applies to all models
    virtual bool needsCrosscheck() const; //!< defect will be present on every model
    virtual bool writeReportToFile() const;

protected:
    bool _needsCrosscheck;

    std::string _formula;
    const char *_arch;
};

//! Checks a given block for "un-deselectable block" defects.
class UndeadBlockDefect : public DeadBlockDefect {
public:
    UndeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const ConfigurationModel *model);
};


#endif
