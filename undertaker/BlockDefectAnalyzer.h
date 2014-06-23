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


/************************************************************************/
/* BlockDefectAnalyzer                                                  */
/************************************************************************/

class BlockDefectAnalyzer {
public:
    enum class DEFECTTYPE { None, Implementation, Configuration, Referential, NoKconfig };

    //!< human readable identifier for the defect type
    const std::string defectTypeToString() const;

    virtual bool isDefect(const ConfigurationModel *model) = 0; //!< checks for a defect
    virtual const std::string getSuffix() const = 0;
    virtual DEFECTTYPE defectType() const       = 0;
    virtual bool isGlobal() const               = 0; //!< return if the defect applies to all models
    virtual void markAsGlobal()                 = 0; //!< mark defect als valid on all models

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

    virtual ~BlockDefectAnalyzer() {}

    static const BlockDefectAnalyzer * analyzeBlock(ConditionalBlock *,
                                                    ConfigurationModel *);
    static std::string getBlockPrecondition(ConditionalBlock *, const ConfigurationModel *);

protected:
    BlockDefectAnalyzer() = default;
    DEFECTTYPE _defectType = DEFECTTYPE::None;
};


/************************************************************************/
/* DeadBlockDefect                                                      */
/************************************************************************/

//! Checks a given block for "un-selectable block" defects.
class DeadBlockDefect : public BlockDefectAnalyzer {
public:
    //! c'tor for Dead Block Defect Analysis
    DeadBlockDefect(ConditionalBlock *);
    virtual bool isDefect(const ConfigurationModel *model) override; //!< checks for a defect
    virtual bool writeReportToFile(bool skip_no_kconfig) const final override;
    virtual void reportMUS()                             const final override;

    virtual const std::string getSuffix()  const final override { return _suffix; }
    virtual DEFECTTYPE defectType()        const final override { return _defectType; }
    //!< return if the defect applies to all models
    virtual bool isGlobal()                const final override { return _isGlobal; }
    //!< mark defect als valid on all models
    virtual void markAsGlobal()                  final override { _isGlobal = true; }

    void setArch(std::string arch) { _arch = arch; }
    const std::string &getArch() const { return _arch; }
    bool isNoKconfigDefect(const ConfigurationModel *model);
    bool needsCrosscheck() const; //!< defect will be present on every model
protected:
    std::string getDefectReportFilename() const;

    bool _isGlobal = false;
    bool _needsCrosscheck = false;
    std::string _suffix;
    std::string _arch;
    std::string _formula;
    std::string _musFormula;
    ConditionalBlock *_cb;
};

/************************************************************************/
/* UndeadBlockDefect                                                    */
/************************************************************************/

//! Checks a given block for "un-deselectable block" defects.
class UndeadBlockDefect : public DeadBlockDefect {
public:
    UndeadBlockDefect(ConditionalBlock *);
    virtual bool isDefect(const ConfigurationModel *model) final override;
};

#endif
