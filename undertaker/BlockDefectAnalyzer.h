// -*- mode: c++ -*-
/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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

#ifndef blockdefect_h__
#define blockdefect_h__

/**
 * \brief Base Class of all Kind of Configuration Defects
 *
 * A BlockDefectAnalyzer checks a CodeSatStream for configuration
 * defects. The known configuration defects are listed in the enum
 * DEFECTTYPE. It provides facilities to test a given block number for a
 * defect and report its results.
 */

#include <string>

class ConditionalBlock;
class ConfigurationModel;


class BlockDefectAnalyzer {
public:
    // FIXME change this to 'enum class DEFECTTYPE' when implementing c++11
    enum DEFECTTYPE { None, Implementation, Configuration, Referential, NoKconfig };

    virtual bool isDefect(const ConfigurationModel *model) = 0; //!< checks for a defect
    virtual bool isGlobal() const = 0; //!< checks if the defect applies to all models
    virtual bool isNoKconfigDefect(const ConfigurationModel *model) = 0;
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
     *
     * By default this method will only create the report if the defect
     * references at least one item that is controlled by the
     * configuration system. Technically, this is implemented by
     * checking if at least one item is inside the configuration space
     * of the given model. This behavior can be overridden with the
     * only_in_model parameter.
     *
     * This method returns 'true' if the file was successfully
     * written. The return code 'false' indicates a file-system
     * permission problem, or if skip_no_kconfig is set to true,
     * indicating that only items in the configurations space will
     * be reported.
     */
    virtual bool writeReportToFile(bool skip_no_kconfig) const = 0;
    virtual void reportMUS() const = 0;
    virtual DEFECTTYPE defectType() const { return _defectType; }
    virtual ~BlockDefectAnalyzer() {}

    static const BlockDefectAnalyzer * analyzeBlock(ConditionalBlock *,
                                                    ConfigurationModel *);

protected:
    BlockDefectAnalyzer() = default;
    DEFECTTYPE _defectType = None;
    bool _isGlobal = false;
    ConditionalBlock *_cb;
    std::string _suffix;
};

//! Checks a given block for "un-selectable block" defects.
class DeadBlockDefect : public BlockDefectAnalyzer {
public:
    //! c'tor for Dead Block Defect Analysis
    DeadBlockDefect(ConditionalBlock *);
    virtual bool isDefect(const ConfigurationModel *model); //!< checks for a defect
    virtual bool isGlobal() const; //!< checks if the defect applies to all models
    virtual bool isNoKconfigDefect(const ConfigurationModel *model);
    virtual bool needsCrosscheck() const; //!< defect will be present on every model
    virtual bool writeReportToFile(bool skip_no_kconfig) const;
    virtual void reportMUS() const;
    virtual const char *getArch() { return _arch; }
    virtual void setArch(const char *arch) { _arch = arch; }

protected:
    std::string getDefectReportFilename() const;
    bool _needsCrosscheck = false;
    const char *_arch = nullptr;
    std::string _formula;
    std::string _musFormula;
};

//! Checks a given block for "un-deselectable block" defects.
class UndeadBlockDefect : public DeadBlockDefect {
public:
    UndeadBlockDefect(ConditionalBlock *);
    virtual bool isDefect(const ConfigurationModel *model);
};

#endif
