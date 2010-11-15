#include <boost/regex.hpp>
#include <fstream>
#include <utility>

#include "StringJoiner.h"
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

std::string CodeSatStream::getMissingItemsConstraints(std::set<std::string> &missing) {
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

std::string CodeSatStream::getCodeConstraints() {
    return std::string((*this).str());
}

std::string CodeSatStream::getKconfigConstraints(const KconfigRsfDb *model,
                                                 std::set<std::string> &missing) {
    std::stringstream ss;

    if (!_doCrossCheck || model->doIntersect(Items(), ss, missing) <= 0)
        return "";

    return std::string(ss.str());
}

const char *CodeSatStream::getParent(const char *block) {
    ParentMap::const_iterator i = parents.find(std::string(block));
    if (i == parents.end())
        return NULL;
    else
        return (*i).second.c_str();
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

void CodeSatStream::analyzeBlock(const char *block) {
    std::set<std::string> missingSet;
    KconfigRsfDb *p_model = 0;

    std::string code_formula = getCodeConstraints();

    std::string kconfig_formula = "";
    std::string missing_formula = "";

    StringJoiner dead_join;
    dead_join.push_back(std::string(block));
    dead_join.push_back(code_formula);

    SatChecker code_constraints(dead_join.join("\n&\n"));

    const char *parent = getParent(block);
    bool has_parent = parent != NULL;

    bool alive = true;
    bool hasMissing = false;

    bool crosscheck = _doCrossCheck;

    if (crosscheck) {
        KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
        p_model = f->lookupModel(_primary_arch);
    }

    if (!code_constraints()) {
        const std::string filename = _filename + "." + block + "." + _primary_arch +".code.globally.dead";
        writePrettyPrinted(filename.c_str(),block, code_constraints.c_str());
        crosscheck = false; // code dead -> no crosscheck
        alive = false;
    } else if (crosscheck){
        /* Can be ""!! */
        kconfig_formula = getKconfigConstraints(p_model, missingSet);
        dead_join.push_back(kconfig_formula);
        SatChecker kconfig_constraints(dead_join.join("\n&\n"));

        /* Can be ""! */
        missing_formula = getMissingItemsConstraints(missingSet);
        dead_join.push_back(missing_formula);
        SatChecker missing_constraints(dead_join.join("\n&\n"));

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

    if (has_parent && alive) {
        std::string undead_block  = "( " + std::string(parent) + " & ! " + std::string(block) + " )";

        StringJoiner undead_join;
        undead_join.push_back(undead_block);

        undead_join.push_back(code_formula);
        SatChecker undead_code_constraints(undead_join.join("\n&\n"));

        undead_join.push_back(kconfig_formula);
        SatChecker undead_kconfig_constraints(undead_join.join("\n&\n"));

        undead_join.push_back(missing_formula);
        SatChecker undead_missing_constraints(undead_join.join("\n&\n"));

        if  (!undead_code_constraints()){
            const std::string filename = _filename + "." + block + "." + _primary_arch +".code.globally.undead";
            writePrettyPrinted(filename.c_str(),block, undead_code_constraints.c_str());
            zombie = false;
        } else if (crosscheck && !zombie) {
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

    if (!crosscheck || !hasMissing)
        return;

    bool dead = !alive;
    if (dead || zombie) {
        KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();

        ModelContainer::iterator i = f->begin();
        std::string dead_missing = "";
        std::string undead_missing = "";

        while ((zombie || dead) && i != f->end()) {
            std::set<std::string> missingSet;
            StringJoiner cross_join;
            KconfigRsfDb *archDb = f->lookupModel((*i).first.c_str());

            cross_join.push_back(code_formula);

            std::string kconfig = getKconfigConstraints(archDb, missingSet);
            cross_join.push_back(kconfig);

            std::string missing = getMissingItemsConstraints(missingSet);
            cross_join.push_back(missing);

            /* We use the joiner twice! */
            cross_join.push_front(block);
            dead_missing = cross_join.join("\n&\n");
            cross_join.pop_front();

            if (dead) {
                /* Joiner must be in the original state afterwards */
                SatChecker missing(dead_missing);
                bool sat = missing();
                /* Alive on one architecture */
                if (sat) {
                    dead = false;
                }
            }

            if (zombie && has_parent) {
                std::string undead_block  = "( " + std::string(parent) + " & ! " + std::string(block) + " )";

                cross_join.push_front(undead_block);
                undead_missing = cross_join.join("\n&\n");

                SatChecker missing_undead(undead_missing);
                bool sat = missing_undead();
                /* Can be dead on one architecture */
                if (sat) {
                    zombie = false;
                }
            }

            /* Next architecture */
            i++;
        }

        const std::string globalf = _filename + "." + block + ".missing.globally.dead";
        const std::string globalu = _filename + "." + block + ".missing.globally.undead";

        /* Dead after crosscheck ? */
        if (dead)
            writePrettyPrinted(globalf.c_str(), block, dead_missing.c_str());

        /* Undead after crosscheck ? */
        if (zombie && has_parent)
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
            analyzeBlock((*i).c_str());
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

std::list<SatChecker::AssignmentMap> CodeSatStream::blockCoverage(KconfigRsfDb *model) {
    std::set<std::string>::iterator i;
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;

    try {
	for(i = _blocks.begin(); i != _blocks.end(); ++i) {
	    std::set<std::string> missingSet;
            StringJoiner formula;

            formula.push_back((*i));
            formula.push_back(getCodeConstraints());
            formula.push_back(getKconfigConstraints(model, missingSet));

	    if (blocks_set.find(*i) == blocks_set.end()) {
                /* does the new contributes to the set of configurations? */
                bool new_solution = false;
                SatChecker sc(formula.join("\n&\n"));

                // unsolvable, i.e. we have found some defect!
                if (!sc())
                    continue;

                SatChecker::AssignmentMap assignments = sc.getAssignment();
                SatChecker::AssignmentMap::const_iterator it;
                for (it = assignments.begin(); it != assignments.end(); it++) {
                    if ((*it).second) {
                        blocks_set.insert((*it).first);
                        new_solution = true;
                    }
                }
                if (new_solution)
                    ret.push_back(assignments);
                //std::cout << "checking coverage for: " << *i << std::endl << formula << std::endl;
            }
        }
    } catch (SatCheckerError &e) {
        std::cerr << "Couldn't process " << _filename << ": "
                  << e.what() << std::endl;
    }
    return ret;
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
