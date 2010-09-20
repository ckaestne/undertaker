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

CodeSatStream::CodeSatStream(std::istream &ifs, std::string filename, const char *primary_arch, std::map<std::string, std::string> pars, bool batch_mode, bool loadModels) 
    : _istream(ifs), _items(), _free_items(), _blocks(), _filename(filename),
      _primary_arch(primary_arch), _doCrossCheck(loadModels), _batch_mode(batch_mode), parents(pars) {
    static const char prefix[] = "CONFIG_";
    static const boost::regex block_regexp("B[0-9]+", boost::regex::perl);
    static const boost::regex comp_regexp("(\\([^\\(]+?[><=!]=.+?\\))", boost::regex::perl);
    static const boost::regex comp_regexp_new("[[:alnum:]]+[[:space:]]*[!]?[><=]+[[:space:]]*[[:alnum:]]+", boost::regex::extended);
    static const boost::regex free_item_regexp("[a-zA-Z0-9\\-_]+", boost::regex::extended);
    std::string line;
    
    //_doCrossCheck = KconfigRsfDbFactory::getInstance()->size() > 0;

    if (!_istream.good())
	return;

    while (std::getline(_istream, line)) {
	//std::cout << "processing line: " << line << std::endl;
	std::string item;
	std::stringstream ss;
	std::string::size_type pos = std::string::npos;
	boost::match_results<std::string::iterator> what;
	boost::match_flag_type flags = boost::match_default;

	while ((pos = line.find("&&")) != std::string::npos)
	    line.replace(pos, 2, 1, '&');

	while ((pos = line.find("||")) != std::string::npos)
	    line.replace(pos, 2, 1, '|');

//	if (boost::regex_search(line.begin(), line.end(), what, comp_regexp, flags)) {
//	    char buf[20];
//	    static int c;
//	    snprintf(buf, sizeof buf, "COMP_%d", c++);
//	    line.replace(what[0].first, what[0].second, buf);
//	}

	if (boost::regex_search(line.begin(), line.end(), what, comp_regexp_new, flags)) {
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
	         //std::cout << "inserting item: " << item << std::endl;
		 continue;
	    } 
	    if (boost::regex_match(item.begin(), item.end(), block_regexp)) {
		_blocks.insert(item);
		continue;
	    }
	    if (boost::regex_match(item.begin(), item.end(), free_item_regexp)) {
	        _free_items.insert(item);
	         //std::cout << "inserting free_item: " << item << std::endl;
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
    //cc << block << " & (1 & ! 0) & " << std::endl << (*this).str();
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
    std::cout << "SLICE:" << Items().size() << ":" << inter << std::endl;
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
    std::cout << "analyzeBlock(" << block << ")" << std::endl; 
    KconfigRsfDbFactory *f = KconfigRsfDbFactory::getInstance();
    std::set<std::string> missingSet;

    std::string formula = getCodeConstraints(block);
    std::string kconfig_formula = "";
    std::string missing_formula = "";
    SatChecker code_constraints(formula);

    std::string parent = parents[std::string(block)]; //this is not good, it inserts an empty string if the block has no parents
    bool has_parent = !parent.empty();

    bool alive = true;

    std::cout << "code_constraints" << std::endl;
    if (!code_constraints()) {
	const std::string filename = _filename + "." + block + "." + _primary_arch +".code.dead";
	writePrettyPrinted(filename.c_str(), code_constraints.c_str());
	alive = false;
    } else if (_doCrossCheck){
        std::cout << _primary_arch << std::endl;
        KconfigRsfDb *p_model = f->lookupModel(_primary_arch);
	kconfig_formula = getKconfigConstraints(block, p_model, missingSet);
        SatChecker kconfig_constraints(kconfig_formula);
	missing_formula = getMissingItemsConstraints(block, p_model, missingSet);
        SatChecker missing_constraints(missing_formula);
        std::cout << "kconfig_constraints" << std::endl;
	if (!kconfig_constraints()) {
	    const std::string filename = _filename + "." + block + "." + _primary_arch +".kconfig.dead";
	    writePrettyPrinted(filename.c_str(), kconfig_constraints.c_str());
	    alive = false;
	} else {
            std::cout << "missing_constraints" << std::endl;
	    if (!missing_constraints()) {
		const std::string filename= _filename + "." + block + "." + _primary_arch +".missing.dead";
		writePrettyPrinted(filename.c_str(), missing_constraints.c_str());
		alive = false;
	    }
	}
    }
    bool zombie = false;
    std::string undead_block= "";
    if (has_parent) {
        std::string undead_block= "( " + parent + " & ! " + std::string(block) + " ) & ";
        std::string undead_code_formula = formula;
        undead_code_formula.replace(0,formula.find('\n',0), undead_block);
        std::string undead_kconfig_formula = kconfig_formula.replace(0,formula.find('\n',0), undead_block); //kaputt
        std::string undead_missing_formula = missing_formula.replace(0,formula.find('\n',0), undead_block); //kaputt
        SatChecker undead_code_constraints(undead_code_formula);
        SatChecker undead_kconfig_constraints(undead_kconfig_formula);
        SatChecker undead_missing_constraints(undead_missing_formula);
        std::cout << "undead_code_constraints" << std::endl;
        if  (!undead_code_constraints()){
	    const std::string filename = _filename + "." + block + "." + _primary_arch +".code.undead";
	    writePrettyPrinted(filename.c_str(), undead_code_constraints.c_str());
	    zombie = true; 
	}
	if (_doCrossCheck & !zombie) {
          std::cout << "undead_kconfig_constraints" << std::endl;
          if  (!undead_kconfig_constraints()){
  	    const std::string filename = _filename + "." + block + "." + _primary_arch +".kconfig.undead";
  	    writePrettyPrinted(filename.c_str(), undead_kconfig_constraints.c_str());
	    zombie = true;
  	  }
          std::cout << "undead_missing_constraints" << std::endl;
          if  (!undead_missing_constraints() & !zombie){
  	    const std::string filename = _filename + "." + block + "." + _primary_arch +".missing.undead";
  	    writePrettyPrinted(filename.c_str(), undead_missing_constraints.c_str());
	    zombie = true;
  	  }
        }
    }
    //std::cout << "Analyzing block: " << block << " on file: " << this->_filename << std::endl;
    //std::cout << formula << std::endl;
    //std::cout << missing_constraints.str();
    runtimes.push_back(std::make_pair(strdup(_filename.c_str()), code_constraints.runtime()));
    
    bool deadDone = false;
    bool zombieDone = !has_parent;
    if (!alive || zombie) {

	if (!_doCrossCheck)
	    return;

	ModelContainer::iterator i;
	for(i = f->begin(); i != f->end(); i++) {
	    std::set<std::string> emptyMissingSet;

	    if ((*i).first.compare(_primary_arch) == 0)
		continue; // skip primary arch

            
            if (!deadDone || !zombieDone) {
              KconfigRsfDb *archDb = f->lookupModel((*i).first.c_str());
	      std::string dead_missing = getMissingItemsConstraints(block,archDb,emptyMissingSet);
	      std::string undead_missing = dead_missing;
	      undead_missing.replace(0,dead_missing.find('\n',0), undead_block);
              SatChecker missing(dead_missing);
              SatChecker missing_undead(undead_missing);
	      if (!deadDone) 
	        deadDone = !missing();
	      if (!zombieDone) 
	        zombieDone = !missing_undead();
	    }

	    if(deadDone && zombieDone) {
		return;
	    }
	}

	const std::string globalf = _filename + "." + block + ".globally.dead";
	const std::string globalu = _filename + "." + block + ".globally.undead";

	if (!deadDone)
	  writePrettyPrinted(globalf.c_str(), (*this).str().c_str());
	if (!zombieDone && has_parent)
	  writePrettyPrinted(globalu.c_str(), (*this).str().c_str());
    }
}


void CodeSatStream::analyzeBlocks() {
    std::set<std::string>::iterator i;
    processed_units++;
    try {
	for(i = _blocks.begin(); i != _blocks.end(); ++i) {
	    analyzeBlock((*i).c_str());
	    //std::cout << "analyzeBlock(" << *i << ")" << std::endl; 
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
    
    if (_batch_mode) {
        //std::cout << SatChecker::pprinter(contents);
	//return true;
    }

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
