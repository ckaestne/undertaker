// -*- mode: c++ -*-
#ifndef blockdefect_h__
#define blockdefect_h__

#include "CodeSatStream.h"

struct BlockDefect {
    enum DEFECTTYPE { None, Implementation, Configuration, Referential };

    virtual bool isDefect(const KconfigRsfDb *model) = 0; /**< checks for an defect */
    virtual bool isGlobal() const = 0; /**< checks if the defect applies to all models */
    virtual bool needsCrosscheck() const = 0; /**< defect will be present on every model */
    virtual void defectIsGlobal();  /**< mark defect als valid on all models */
    virtual bool writeReportToFile() const = 0;
    virtual void markOk(const std::string &arch);
    virtual std::list<std::string> getOKList() { return _OKList; }
    virtual int defectType() const { return _defectType; }
    virtual ~BlockDefect() {}
protected:
    BlockDefect(int defecttype) : _defectType(defecttype), _isGlobal(false), _OKList() {}
    int _defectType;
    bool _isGlobal;
    std::list<std::string> _OKList; /**< List of architectures on which this is proved to be OK */
};

class DeadBlockDefect : public BlockDefect {
public:
    DeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const KconfigRsfDb *model); /**< checks for an defect */
    virtual bool isGlobal() const; /**< checks if the defect applies to all models */
    virtual bool needsCrosscheck() const; /**< defect will be present on every model */
    /**
     * \brief Write out a report to a file.
     *
     * Write out a report to a file. The Filename is calculated based on the
     * defect type.  Examples:
     *
     * filename                    |  meaning: dead because...
     * ---------------------------------------------------------------------------------
     * $block.$arch.code.dead     -> only considering the CPP structure and expressions
     * $block.$arch.kconfig.dead  -> additionally considering kconfig constraints
     * $block.$arch.missing.dead  -> additionally setting symbols not found in kconfig
     *                               to false (grounding these variables)
     * $block.globally.dead       -> dead on every checked arch
     */
    virtual bool writeReportToFile() const;

protected:
    const std::string getCodeConstraints() const;
    const std::string getKconfigConstraints(const KconfigRsfDb *model, std::set<std::string> &missing) const;
    const std::string getMissingItemsConstraints(std::set<std::string> &missing) const;

    CodeSatStream *_cs;
    const char *_block;
    bool _needsCrosscheck;

    std::string _formula;
    const char *_arch;
    const char *_suffix;
};

class UndeadBlockDefect : public DeadBlockDefect {
public:
    UndeadBlockDefect(CodeSatStream *cs, const char *block);
    virtual bool isDefect(const KconfigRsfDb *model);
};


#endif
