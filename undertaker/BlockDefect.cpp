#include "StringJoiner.h"
#include "BlockDefect.h"
#include "ModelContainer.h"


void BlockDefect::markOk(const std::string &arch) {
    _OKList.push_back(arch);
}

DeadBlockDefect::DeadBlockDefect(CodeSatStream *cs, const char *block)
    :  BlockDefect(None), _cs(cs), _block(block), _needsCrosscheck(false),
       _arch(NULL), _suffix("dead") {}

bool DeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;

    if(!_arch)
        _arch = ModelContainer::lookupArch(model);

    formula.push_back(_block);
    formula.push_back(getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(this->getKconfigConstraints(model, missingSet));
        SatChecker kconfig_constraints(_formula = formula.join("\n&&\n"));

        if (!kconfig_constraints()) {
            _defectType = Configuration;
            _isGlobal = true;
            return true;
        } else {
            formula.push_back(this->getMissingItemsConstraints(missingSet));
            SatChecker missing_constraints(_formula = formula.join("\n&&\n"));

            if (!missing_constraints()) {
                _defectType = Referential;
                return true;
            }
        }
    }
    return false;
}

bool DeadBlockDefect::isGlobal() const { return _isGlobal; }
bool DeadBlockDefect::needsCrosscheck() const {
    switch(_defectType) {
    case None:
    case Implementation:
    case Configuration:
        return false;
    default:
        // skip crosschecking if we already know that it is global
        return !_isGlobal;
    }
}

void BlockDefect::defectIsGlobal() { _isGlobal = true; }

const std::string DeadBlockDefect::getCodeConstraints() const {
    return _cs->getCodeConstraints();
}

const std::string DeadBlockDefect::getKconfigConstraints(const ConfigurationModel *model,
                                                           std::set<std::string> &missing) const{
    return _cs->getKconfigConstraints(model, missing);
}

const std::string DeadBlockDefect::getMissingItemsConstraints(std::set<std::string> &missing) const {
    return _cs->getMissingItemsConstraints(missing);
}

bool DeadBlockDefect::writeReportToFile() const {
    StringJoiner fname_joiner;

    fname_joiner.push_back(_cs->getFilename());
    fname_joiner.push_back(_block);
    if(_arch && (!_isGlobal || _defectType == Configuration))
        fname_joiner.push_back(_arch);

    switch(_defectType) {
    case None: return false;  // Nothing to write
    case Implementation:
        fname_joiner.push_back("code");
        break;
    case Configuration:
        fname_joiner.push_back("kconfig");
        break;
    case Referential:
        fname_joiner.push_back("missing");
        break;
    }

    if (_isGlobal)
        fname_joiner.push_back("globally");

    fname_joiner.push_back(_suffix);

    const std::string filename = fname_joiner.join(".");
    const std::string &contents = _formula;

    std::ofstream out(filename.c_str());

    if (!out.good()) {
        std::cerr << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        std::cout << "I: creating " << filename << std::endl;
        out << "#" << _block << ":" << _cs->getLine(_block) << std::endl;
        out << SatChecker::pprinter(contents.c_str());
        out.close();
    }

    return true;
}


UndeadBlockDefect::UndeadBlockDefect(CodeSatStream *cs, const char *block)
    : DeadBlockDefect(cs, block) { this->_suffix = "undead"; }

bool UndeadBlockDefect::isDefect(const ConfigurationModel *model) {
    StringJoiner formula;
    const char *parent = _cs->getParent(_block);

    // no parent -> impossible to be undead
    if (!parent)
        return false;

    if (!_arch)
        _arch = ModelContainer::lookupArch(model);

    formula.push_back("( " + std::string(parent) + " && ! " + std::string(_block) + " )");
    formula.push_back(getCodeConstraints());
    SatChecker code_constraints(_formula = formula.join("\n&&\n"));

    if (!code_constraints()) {
        _defectType = Implementation;
        _isGlobal = true;
        return true;
    }

    if (model) {
        std::set<std::string> missingSet;
        formula.push_back(getKconfigConstraints(model, missingSet));
        SatChecker kconfig_constraints(_formula = formula.join("\n&&\n"));

        if (!kconfig_constraints()) {
            _defectType = Configuration;
            _isGlobal = true;
            return true;
        } else {
            formula.push_back(this->getMissingItemsConstraints(missingSet));
            SatChecker missing_constraints(_formula = formula.join("\n&&\n"));

            if (!missing_constraints()) {
                _defectType = Referential;
                return true;
            }
        }
    }
    return false;
}
