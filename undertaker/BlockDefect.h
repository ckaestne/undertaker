// -*- mode: c++ -*-
#ifndef blockdefect_h__
#define blockdefect_h__

#include "CodeSatStream.h"

/**
 * \brief Base Class of all Kind of Configuration Defects
 *
 * A BlockDefect is an Analyzer classe, that checks a CodeSatStream for
 * configuration defects. The known configuration defects are listed in
 * the enum DEFECTTYPE. It provides facilities to test a given block
 * number for a defect and report its results.
 */
struct BlockDefect {
    enum DEFECTTYPE { None, Implementation, Configuration, Referential };

    virtual bool isDefect(const ConfigurationModel *model) = 0; //!< checks for a defect
    virtual bool isGlobal() const = 0; //!< checks if the defect applies to all models
    virtual bool needsCrosscheck() const = 0; //!< defect will be present on every model
    virtual void defectIsGlobal();  //!< mark defect als valid on all models
    const std::string defectTypeToString() const; //!< human readable identifier for the defect type

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
    virtual ~BlockDefect() {}
protected:
    BlockDefect(int defecttype) : _defectType(defecttype), _isGlobal(false), _OKList() {}
    int _defectType;
    bool _isGlobal;
    std::list<std::string> _OKList; //!< List of architectures on which this is proved to be OK
};

//! Checks a given block for "un-selectable block" defects.
class DeadBlockDefect : public BlockDefect {
public:
    //! c'tor for Dead Block Defect Analysis
    DeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const ConfigurationModel *model); //!< checks for a defect
    virtual bool isGlobal() const; //!< checks if the defect applies to all models
    virtual bool needsCrosscheck() const; //!< defect will be present on every model
    virtual bool writeReportToFile() const;

protected:
    const std::string getCodeConstraints() const;
    const std::string getKconfigConstraints(const ConfigurationModel *model, std::set<std::string> &missing) const;
    const std::string getMissingItemsConstraints(std::set<std::string> &missing) const;

    CodeSatStream *_cs;
    const char *_block;
    bool _needsCrosscheck;

    std::string _formula;
    const char *_arch;
    const char *_suffix;
};

//! Checks a given block for "un-deselectable block" defects.
class UndeadBlockDefect : public DeadBlockDefect {
public:
    UndeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const ConfigurationModel *model);
};


#endif
