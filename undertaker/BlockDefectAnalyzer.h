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

    /**
     * \brief helper functions to remove files matching a pattern
     *
     * \param pattern the pattern to match
     * \return the number of files removed
     */
    static int rmPattern(const char *pattern);

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

    //! removes reports that may have been created from previous runs. returns number of files removed
    static int removeOldReports(const char *filename);

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
