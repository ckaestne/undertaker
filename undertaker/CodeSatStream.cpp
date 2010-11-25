#include <boost/regex.hpp>
#include <fstream>
#include <utility>

#include "StringJoiner.h"
#include "KconfigRsfDbFactory.h"
#include "ConfigurationModel.h"
#include "KconfigWhitelist.h"
#include "CodeSatStream.h"
#include "BlockDefect.h"
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

        while (boost::regex_search(line.begin(), line.end(), what, comp_regexp_new, flags)) {
            char buf[20];
            static int c;
            snprintf(buf, sizeof buf, "COMP_%d", c++);
            line.replace(what[0].first, what[0].second, buf);
        }

        while ((pos = line.find("defined")) != std::string::npos)
            line.erase(pos,7);

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
            m << " || " << (*it) ;
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

std::string CodeSatStream::getKconfigConstraints(const ConfigurationModel *model,
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

/**
 * \param block    block to check
 * \param p_model  primary model to check against
 */
const BlockDefect* CodeSatStream::analyzeBlock(const char *block, ConfigurationModel *p_model) {

    BlockDefect *defect = new DeadBlockDefect(this, block);

    // If this is neither an Implementation, Configuration nor Referential *dead*,
    // then destroy the analysis and retry with an Undead Analysis
    if(!defect->isDefect(p_model)) {
        delete defect;
        defect = new UndeadBlockDefect(this, block);

        // No defect found, block seems OK
        if(!defect->isDefect(p_model)) {
            delete defect;
            return NULL;
        }
    }

    assert(defect->defectType() != BlockDefect::None);

    // (ATM) Implementation and Configuration defects do not require a crosscheck
    if (!_doCrossCheck || !defect->needsCrosscheck())
        return defect;

    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    for (ModelContainer::iterator i = f->begin(); i != f->end(); i++) {
        const std::string &arch = (*i).first;
        const ConfigurationModel *model = (*i).second;

        if (!defect->isDefect(model)) {
            defect->markOk(arch);
            return defect;
        }
    }
    defect->defectIsGlobal();
    return defect;
}

void CodeSatStream::analyzeBlocks() {
    ConfigurationModel *p_model = 0;

    if (_doCrossCheck) {
        KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
        p_model = f->lookupModel(_primary_arch);
    }
    
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
            const BlockDefect *defect = analyzeBlock((*i).c_str(), p_model);
            if (defect) {
                defect->writeReportToFile();
                delete defect;
            }
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

std::list<SatChecker::AssignmentMap> CodeSatStream::blockCoverage(ConfigurationModel *model) {
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
                SatChecker sc(formula.join("\n&&\n"));

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

std::string CodeSatStream::getLine(const char *block) const {
    if (this->_cc)
        return this->_cc->getPosition(block);
    else
        throw std::runtime_error("no cloud container");
}
