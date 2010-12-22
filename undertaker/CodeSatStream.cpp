#include <algorithm> // std::transform
#include <cctype> // std::toupper
#include <fstream>
#include <utility>

#include <boost/regex.hpp>

#include "StringJoiner.h"
#include "ModelContainer.h"
#include "ConfigurationModel.h"
#include "CodeSatStream.h"
#include "BlockDefectAnalyzer.h"
#include "SatChecker.h"
#include "Ziz.h"

unsigned int CodeSatStream::processed_units;
unsigned int CodeSatStream::processed_items;
unsigned int CodeSatStream::processed_blocks;
unsigned int CodeSatStream::failed_blocks;


CodeSatStream::CodeSatStream(std::istream &ifs, std::string filename,
                             ParentMap pars, BlockCloud *cc,
                             bool batch_mode, bool loadModels)
    : _istream(ifs), _items(), _free_items(), _blocks(), _filename(filename),
      _doCrossCheck(loadModels), _cc(cc), _batch_mode(batch_mode), parents(pars) {

    static const char prefix[] = "CONFIG_";
    static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
    static const boost::regex comp_regexp("[_[:alnum:]]+[[:space:]]*[!]?[><=]+[[:space:]]*[_[:alnum:]]+",
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

        while (boost::regex_search(line.begin(), line.end(), what, comp_regexp, flags)) {
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
const BlockDefectAnalyzer* CodeSatStream::analyzeBlock(const char *block, ConfigurationModel *p_model) {

    BlockDefectAnalyzer *defect = new DeadBlockDefect(this, block);

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

    assert(defect->defectType() != BlockDefectAnalyzer::None);

    // (ATM) Implementation and Configuration defects do not require a crosscheck
    if (!_doCrossCheck || !defect->needsCrosscheck())
        return defect;

    ModelContainer *f = ModelContainer::getInstance();
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
        ModelContainer *f = ModelContainer::getInstance();
        p_model = f->lookupMainModel();
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
            const BlockDefectAnalyzer *defect = analyzeBlock((*i).c_str(), p_model);
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

static int lineFromPosition(std::string line) {
    // INPUT: foo:121:2
    // OUTPUT: 121

    size_t start = line.find_first_of(':');
    assert (start != line.npos);
    size_t stop = line.find_first_of(':', start);
    assert (stop != line.npos);

    std::stringstream ss(line.substr(start+1, stop));
    int i;
    ss >> i;
    return i;
}

static int lineFromPosition(position_type line) {
    std::stringstream ss;
    ss << line;
    return lineFromPosition(ss.str());
}


std::string CodeSatStream::positionToBlock(std::string position) {
    std::set<std::string>::iterator i;

    int line = lineFromPosition(position);
    std::string block;
    int block_length = -1;

    for(i = _blocks.begin(); i != _blocks.end(); ++i) {
        BlockCloud::index index = _cc->search(*i);
        ZizCondBlockPtr &ziz_block = _cc->at(index);
        const Ziz::ConditionalBlock *cond_block = ziz_block.Block();
        int begin = lineFromPosition(cond_block->Start());
        int last = lineFromPosition(cond_block->End());

        if (last < begin) continue;
        /* Found a short block, using this one */
        if ((((last - begin) < block_length) || block_length == -1)
            && begin < line
            && line < last) {
            block = *i;
            block_length = last - begin;
        }
    }
    return block;
}


std::list<SatChecker::AssignmentMap> CodeSatStream::blockCoverage(ConfigurationModel *model, MissingSet &missingSet) {
    std::set<std::string>::iterator i;
    std::set<std::string> blocks_set;
    std::list<SatChecker::AssignmentMap> ret;
    std::set<SatChecker::AssignmentMap> found_solutions;

    StringList empty;
    StringList &sl = empty;

    try {
        const std::string magic("ALWAYS_ON");
        RsfMap::const_iterator j = model->find(magic);
        if (j != model->end()) {
            sl = (*j).second;
            std::cout << "I: " << sl.size() << " Items have been forcefully set" << std::endl;
        }
        
	for(i = _blocks.begin(); i != _blocks.end(); ++i) {
            StringJoiner formula;
            SatChecker::AssignmentMap current_solution;

            formula.push_back((*i));
            formula.push_back(getCodeConstraints());
            formula.push_back(getKconfigConstraints(model, missingSet));

            for (MissingSet::iterator it = missingSet.begin(); it != missingSet.end(); it++)
                formula.push_back("!" + (*it));

            std::string main_arch(ModelContainer::getMainModel());
            if (main_arch.size() > 0) {
                // stolen from http://www.codepedia.com/1/CppToUpperCase
                // explicit cast needed to resolve ambiguity
                std::transform(main_arch.begin(), main_arch.end(), main_arch.begin(),
                               (int(*)(int))std::toupper);
                formula.push_back("CONFIG_" + main_arch);
            }

            for (StringList::const_iterator sli=sl.begin(); sli != sl.end(); ++sli) {
                formula.push_back(*sli);
            }

	    if (blocks_set.find(*i) == blocks_set.end()) {
                /* does the new contributes to the set of configurations? */
                bool new_solution = false;
                SatChecker sc(formula.join("\n&&\n"));

                // unsolvable, i.e. we have found some defect!
                if (!sc())
                    continue;

                const SatChecker::AssignmentMap &assignments = sc.getAssignment();
                SatChecker::AssignmentMap::const_iterator it;
                for (it = assignments.begin(); it != assignments.end(); it++) {
                    static const boost::regex item_regexp("^CONFIG_(.*)$", boost::regex::perl);
                    static const boost::regex block_regexp("^B\\d+$", boost::regex::perl);

                    if (boost::regex_match((*it).first, item_regexp)) {
                        current_solution.insert(std::make_pair<std::string,bool>((*it).first, (*it).second));
                        continue;
                    }

                    if ((*it).second && boost::regex_match((*it).first, block_regexp)) {
                        blocks_set.insert((*it).first);
                        new_solution = true;
                    }
                }

                if (found_solutions.find(current_solution) == found_solutions.end()) {
                    found_solutions.insert(current_solution);

                    if (new_solution)
                        ret.push_back(assignments);
                }
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
