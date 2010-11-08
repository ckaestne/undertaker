#include <boost/regex.hpp>
#include <fstream>
#include <utility>


#include "KconfigRsfDbFactory.h"
#include "KconfigRsfDb.h"
#include "KconfigWhitelist.h"
#include "CodeSatStream.h"
#include "SatChecker.h"
#include "Ziz.h"

unsigned int CodeSatStream::processed_units;
unsigned int CodeSatStream::processed_items;
unsigned int CodeSatStream::processed_blocks;
unsigned int CodeSatStream::failed_blocks;

//static RuntimeTable runtimes;

CodeSatStream::CodeSatStream(std::istream &ifs, std::string filename, const char *primary_arch,
                             ParentMap pars, BlockCloud *cc,
                             bool batch_mode, bool loadModels)
    : _istream(ifs), _items(), _free_items(), _blocks(), _filename(filename),
      _primary_arch(primary_arch), _doCrossCheck(loadModels), _cc(cc),
      _batch_mode(batch_mode), parents(pars) {

    static const char prefix[] = "CONFIG_";
    static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
    static const boost::regex comp_regexp("(\\([^\\(]+?[><=!]=.+?\\))", boost::regex::perl);
    static const boost::regex comp_regexp_new("[[:alnum:]]+[[:space:]]*[!]?[><=]+[[:space:]]*[[:alnum:]]+",
                                              boost::regex::extended);
    static const boost::regex free_item_regexp("[a-zA-Z0-9\\-_]+", boost::regex::extended);
    std::string line;

    if (!_istream.good())
        return;

    while (std::getline(_istream, line)) {
        std::string item;
        std::stringstream ss;
        std::string::size_type pos = std::string::npos;
        boost::match_results<std::string::iterator> what;
        boost::match_flag_type flags = boost::match_default;

        while ((pos = line.find("&&")) != std::string::npos)
            line.replace(pos, 2, 1, '&');

        while ((pos = line.find("||")) != std::string::npos)
            line.replace(pos, 2, 1, '|');

        while (boost::regex_search(line.begin(), line.end(), what, comp_regexp_new, flags)) {
            char buf[20];
            static int c;
            snprintf(buf, sizeof buf, "COMP_%d", c++);
            line.replace(what[0].first, what[0].second, buf);
        }

        while ((pos = line.find("defined")) != std::string::npos)
            line.erase(pos,7);

        while ((pos = line.find("&&")) != std::string::npos)
            line.replace(pos, 2, 1, '&');

        while ((pos = line.find("||")) != std::string::npos)
            line.replace(pos, 2, 1, '|');

        ss.str(line);

        while (ss >> item) {
            if ((pos = item.find(prefix)) != std::string::npos) { // i.e. matched
                _items.insert(item);
                continue;
            }
            if (boost::regex_match(item.begin(), item.end(), block_regexp)) {
                _blocks.insert(item);
                continue;
            }
            if (boost::regex_match(item.begin(), item.end(), free_item_regexp)) {
                _free_items.insert(item);
            }
        }

        (*this) << line << std::endl;
    }
}

std::string CodeSatStream::buildTermMissingItems(std::set<std::string> missing) const {
    std::stringstream m;
    for(std::set<std::string>::iterator it = missing.begin(); it != missing.end(); it++) {
        if (it == missing.begin()) {
            m << "( ! ( " << (*it);
        } else {
            m << " | " << (*it) ;
        }
    }
    if (!m.str().empty()) {
        m << " ) )";
    }
    return m.str();
}

std::string CodeSatStream::getCodeConstraints(const char *block) {
    std::stringstream cc;
    cc << block << " &" << std::endl << (*this).str();
    return std::string(cc.str());
}

int SLICE_fixit;

std::string CodeSatStream::getKconfigConstraints(const char *block,
                                                 const KconfigRsfDb *model,
                                                 std::set<std::string> &missing) {
    std::stringstream ss;
    std::stringstream kc;
    std::string code = this->getCodeConstraints(block);
    int slice = -1;
    int inter = model->doIntersect(Items(), ss, missing, slice);
    SLICE_fixit = slice;
    if (inter > 0) {
        kc << code << std::endl << " & " << std::endl << ss.str();
    } else {
        return code;
    }
    //    std::cout << "SLICE:" << Items().size() << ":" << inter << std::endl;
    return std::string(kc.str());
}

std::string CodeSatStream::getMissingItemsConstraints(const char *block,
                                                      const KconfigRsfDb *model,
                                                      std::set<std::string> &missing) {
    std::string kc = this->getKconfigConstraints(block, model, missing);
    std::string missingTerm = buildTermMissingItems(missing);
    std::stringstream kcm;
    if (!missingTerm.empty()) {
        kcm << kc << std::endl << " & " << std::endl << missingTerm;
    } else {
        return kc;
    }

    return std::string(kcm.str());
}

/*
 * possible files created based on findings:
 *
 * filename                    |  meaning: dead because...
 * ---------------------------------------------------------------------------------
 * $block.$arch.code.dead     -> only considering the CPP structure and expressions
 * $block.$arch.kconfig.dead  -> additionally considering kconfig constraints
 * $block.$arch.missing.dead  -> additionally setting symbols not found in kconfig
 *                               to false (grounding these variables)
 * $block.globally.dead       -> dead on every checked arch
 */

void CodeSatStream::analyzeBlock(const char *block, RuntimeEntry &re) {
    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    std::set<std::string> missingSet;

    std::string formula = getCodeConstraints(block);
    std::string kconfig_formula = "";
    std::string missing_formula = "";
    SatChecker code_constraints(formula);

    std::string parent = parents[std::string(block)]; //this is not good, it inserts an empty string if the block has no parents
    bool has_parent = !parent.empty();

    bool alive = true;
    bool hasMissing = false;

    if (!code_constraints()) {
        const std::string filename = _filename + "." + block + "." + _primary_arch +".code.globally.dead";
        writePrettyPrinted(filename.c_str(),block, code_constraints.c_str());
        alive = false;
    } else if (_doCrossCheck){
        KconfigRsfDb *p_model = f->lookupModel(_primary_arch);
        kconfig_formula = getKconfigConstraints(block, p_model, missingSet);
        SatChecker kconfig_constraints(kconfig_formula);
        re.slice = SLICE_fixit; //fucking ugly!!!! fix it!

        missing_formula = getMissingItemsConstraints(block, p_model, missingSet);
        SatChecker missing_constraints(missing_formula);
        if (!kconfig_constraints()) {
            const std::string filename = _filename + "." + block + "." + _primary_arch +".kconfig.globally.dead";
            writePrettyPrinted(filename.c_str(),block, kconfig_constraints.c_str());
            alive = false;
        } else {
            if (!missing_constraints()) {
                const std::string filename= _filename + "." + block + "." + _primary_arch +".missing.dead";
                writePrettyPrinted(filename.c_str(),block, missing_constraints.c_str());
                alive = false;
                hasMissing = true;
            }
        }
    }
    bool zombie = false;
    std::string undead_block= "( " + parent + " & ! " + std::string(block) + " ) & ";

    if (has_parent && alive) {
        std::string undead_code_formula = formula;
        undead_code_formula.replace(0,formula.find('\n',0), undead_block);
        std::string undead_kconfig_formula = kconfig_formula.replace(0,formula.find('\n',0), undead_block); //kaputt
        std::string undead_missing_formula = missing_formula.replace(0,formula.find('\n',0), undead_block); //kaputt
        SatChecker undead_code_constraints(undead_code_formula);
        SatChecker undead_kconfig_constraints(undead_kconfig_formula);
        SatChecker undead_missing_constraints(undead_missing_formula);
        if  (!undead_code_constraints()){
            const std::string filename = _filename + "." + block + "." + _primary_arch +".code.globally.undead";
            writePrettyPrinted(filename.c_str(),block, undead_code_constraints.c_str());
            zombie = true;
        } else if (_doCrossCheck && !zombie) {
            if  (!undead_kconfig_constraints()){
                const std::string filename = _filename + "." + block + "." + _primary_arch +".kconfig.globally.undead";
                writePrettyPrinted(filename.c_str(),block, undead_kconfig_constraints.c_str());
                zombie = true;
            } else {
                if  (!undead_missing_constraints() & !zombie){
                    const std::string filename = _filename + "." + block + "." + _primary_arch +".missing.undead";
                    writePrettyPrinted(filename.c_str(),block, undead_missing_constraints.c_str());
                    zombie = true;
                    hasMissing = true;
                }
            }
        }
    }

    bool deadDone = false;
    bool zombieDone = false;
    bool dead = !alive;
    std::string dead_missing = "";
    std::string undead_missing = "";
    if (dead || zombie) {

        if (!_doCrossCheck || !hasMissing)
            return;

        ModelContainer::iterator i;
        for(i = f->begin(); i != f->end(); i++) {
            std::set<std::string> emptyMissingSet;

            if (!deadDone || !zombieDone) {
                KconfigRsfDb *archDb = f->lookupModel((*i).first.c_str());
                dead_missing = getMissingItemsConstraints(block,archDb,emptyMissingSet);
                undead_missing = dead_missing;
                undead_missing.replace(0,dead_missing.find('\n',0), undead_block);
                SatChecker missing(dead_missing);
                SatChecker missing_undead(undead_missing);
                if (!deadDone) {
                    bool sat = missing();
                    if (sat) {
                        deadDone = true;
                    }
                }
                if (!zombieDone && has_parent) {
                    bool sat = missing_undead();
                    if (sat) {
                        zombieDone = true;
                    }
                }
            } else {
                return;
            }
        }

        const std::string globalf = _filename + "." + block + ".missing.globally.dead";
        const std::string globalu = _filename + "." + block + ".missing.globally.undead";

        if (!deadDone)
            writePrettyPrinted(globalf.c_str(), block, dead_missing.c_str());

        if (!zombieDone && has_parent)
            writePrettyPrinted(globalu.c_str(), block, undead_missing.c_str());
    }
}


void CodeSatStream::analyzeBlocks() {
    std::set<std::string>::iterator i;
    processed_units++;
    try {
        for(i = _blocks.begin(); i != _blocks.end(); ++i) {
            clock_t start, end;

            RuntimeEntry re;
            re.filename = _filename;
            re.cloud = *_blocks.begin();
            re.block = *i;
            re.i_items = this->Items().size();

            start = clock();
            analyzeBlock((*i).c_str(), re);
            end = clock();

            re.rt_full_analysis = end - start;
            this->runtimes.push_back(re);
            processed_blocks++;
        }
        processed_items += _items.size();
    } catch (SatCheckerError &e) {
        failed_blocks++;
        std::cerr << "Couldn't process " << _filename << ": "
                  << e.what() << std::endl;
    }
}

void CodeSatStream::blockCoverage() {
    std::set<std::string>::iterator i;
    std::set<std::string> blocks_set;
    int sat_calls = 0;
    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();

    try {
	for(i = _blocks.begin(); i != _blocks.end(); ++i) {
	    KconfigRsfDb *p_model = f->lookupModel(_primary_arch);
            std::string formula = (*i) + " & " ;
	    //formula += getCodeConstraints((*i).c_str());
	    std::set<std::string> missingSet;
	    formula += getKconfigConstraints((*i).c_str(), p_model, missingSet);
	    if (blocks_set.find(*i) == blocks_set.end()) {
                SatChecker sc(formula);

                // unsolvable, i.e. we have found some defect!
                if (!sc())
                    continue;

                sat_calls++;
                SatChecker::AssignmentMap assignments = sc.getAssignment();
                SatChecker::AssignmentMap::const_iterator it;
                for (it = assignments.begin(); it != assignments.end(); it++) {
                    if ((*it).second)
                        blocks_set.insert((*it).first);
                }
                //std::cout << "checking coverage for: " << *i << std::endl << formula << std::endl; 
	    }
	}
	std::cout << "blocks: " << _blocks.size() << ", sat_calls: " << sat_calls << ", "
                  << "blocks_set: " << blocks_set.size() << std::endl;
    } catch (SatCheckerError &e) {
	std::cerr << "Couldn't process " << _filename << ": "
		  << e.what() << std::endl;
    }
}

bool CodeSatStream::dumpRuntimes() {
    std::list<RuntimeEntry>::iterator it;
    for ( it=this->runtimes.begin() ; it != this->runtimes.end(); it++ )
        std::cout << (*it).getString();

    return true;
}


bool CodeSatStream::writePrettyPrinted(const char *filename, std::string block, const char *contents) const {
    KconfigWhitelist *wl = KconfigWhitelist::getInstance();
    const char *wli = wl->containsWhitelistedItem(contents);

    std::ofstream out(filename);

    if (wli) {
        std::cout << "I: not creating " << filename 
                  << " (contains whitelisted item: " << wli << ")"
                  << std::endl;
        return false;
    }

    if (!out.good()) {
        std::cerr << "failed to open " << filename << " for writing " << std::endl;
        return false;
    } else {
        std::cout << "I: creating " << filename << std::endl;
        out << "#" << block << ":" << getLine(block) << std::endl;
        out << SatChecker::pprinter(contents);
        out.close();
    }
    return true;
}

std::string CodeSatStream::getLine(std::string block) const {
    if (this->_cc)
        return this->_cc->getPosition(block);
    else
        throw std::runtime_error("no cloud container");
}
