#include <boost/regex.hpp>
#include <fstream>
#include <utility>


#include "KconfigRsfDbFactory.h"
#include "KconfigRsfDb.h"
#include "CodeSatStream.h"
#include "SatChecker.h"

unsigned int CodeSatStream::processed_units;
unsigned int CodeSatStream::processed_items;
unsigned int CodeSatStream::processed_blocks;
unsigned int CodeSatStream::failed_blocks;

static RuntimeTable runtimes;

CodeSatStream::CodeSatStream(std::istream &ifs, std::string filename, const char *primary_arch, bool batch_mode) 
    : _istream(ifs), _items(), _blocks(), _filename(filename),
      _primary_arch(primary_arch), _doCrossCheck(true), _batch_mode(batch_mode) {
    static const char prefix[] = "CONFIG_";
    static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
    static const boost::regex comp_regexp("(\\([^\\(]+?[><=!]=.+?\\))", boost::regex::perl);
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

	if (boost::regex_search(line.begin(), line.end(), what, comp_regexp, flags)) {
	    char buf[20];
	    static int c;
	    snprintf(buf, sizeof buf, "COMP_%d", c++);
	    line.replace(what[0].first, what[0].second, buf);
	}

	while ((pos = line.find("&&")) != std::string::npos)
	    line.replace(pos, 2, 1, '&');

	while ((pos = line.find("||")) != std::string::npos)
	    line.replace(pos, 2, 1, '|');

	ss.str(line);

	while (ss >> item) {
	    if ((pos = item.find(prefix)) != std::string::npos) { // i.e. matched
		_items.insert(item);
	    } 
	    if (boost::regex_match(item.begin(), item.end(), block_regexp)) {
		_blocks.insert(item);
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
    cc << block << " & " << std::endl << (*this).str();
    return std::string(cc.str());
}

std::string CodeSatStream::getKconfigConstraints(const char *block,
						 const KconfigRsfDb *model,
						 std::set<std::string> &missing) {
    std::stringstream ss;
    std::stringstream kc;
    std::string code = this->getCodeConstraints(block);
    int inter = model->doIntersect(Items(), ss, missing);
    if (inter > 0) {
	kc << code << std::endl << " & " << std::endl << ss.str();
    } else {
	return code;
    }
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

void CodeSatStream::analyzeBlock(const char *block) {
    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    std::set<std::string> missingSet;
    KconfigRsfDb *p_model = f->lookupModel(_primary_arch);
    SatChecker code_constraints(getCodeConstraints(block));
    SatChecker kconfig_constraints(getKconfigConstraints(block, p_model, missingSet));
    SatChecker missing_constraints(getMissingItemsConstraints(block, p_model, missingSet));
    bool alive = true;

    if (!code_constraints()) {
	const std::string filename = _filename + "." + block + "." + _primary_arch +".code.dead";
	writePrettyPrinted(filename.c_str(), code_constraints.c_str());
	alive = false;
    } else {
	if (!kconfig_constraints()) {
	    const std::string filename = _filename + "." + block + "." + _primary_arch +".kconfig.dead";
	    writePrettyPrinted(filename.c_str(), kconfig_constraints.c_str());
	    alive = false;
	} else {
	    if (!missing_constraints()) {
		const std::string filename= _filename + "." + block + "." + _primary_arch +".missing.dead";
		writePrettyPrinted(filename.c_str(), missing_constraints.c_str());
		alive = false;
	    }
	}
    }
    std::cout << missing_constraints.str();
    runtimes.push_back(std::make_pair(strdup(_filename.c_str()), code_constraints.runtime()));
    
    if (!alive) {

	if (!_doCrossCheck)
	    return;

	ModelContainer::iterator i;
	for(i = f->begin(); i != f->end(); i++) {
	    std::set<std::string> emptyMissingSet;

	    if ((*i).first.compare(_primary_arch) == 0)
		continue; // skip primary arch

	    KconfigRsfDb *archDb = f->lookupModel((*i).first.c_str());
            SatChecker missing(getMissingItemsConstraints(block,archDb,emptyMissingSet));

	    if(missing()) {
		return;
	    }
	}

	const std::string globalf = _filename + "." + block + ".globally.dead";
	writePrettyPrinted(globalf.c_str(), (*this).str().c_str());
    }
}


void CodeSatStream::analyzeBlocks() {
    std::set<std::string>::iterator i;
    processed_units++;
    try {
	for(i = _blocks.begin(); i != _blocks.end(); ++i) {
	    analyzeBlock((*i).c_str());
	    processed_blocks++;
	}
	processed_items += _items.size();
    } catch (SatCheckerError &e) {
	failed_blocks++;
	std::cerr << "Couldn't process " << _filename << ": "
		  << e.what() << std::endl;
    }
}

bool CodeSatStream::writePrettyPrinted(const char *filename, const char *contents) const {
    std::ofstream out(filename);

    if (!out.good()) {
	std::cerr << "failed to open " << filename << " for writing " << std::endl;
	return false;
    } else {
	std::cout << "creating " << filename << std::endl;
	out << SatChecker::pprinter(contents);
	out.close();
    }
    return true;
}


const RuntimeTable &CodeSatStream::getRuntimes() {
    return runtimes;
}
