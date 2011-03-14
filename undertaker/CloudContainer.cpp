/*
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2011 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2009-2011 Julio Sincero <Julio.Sincero@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2011 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <sstream>
#include <boost/regex.hpp>

#include "StringJoiner.h"
#include "CloudContainer.h"
#include "Ziz.h"

const char *ZizCondBlockPtr::expression() const {

    if (_expression)
        return _expression;

    std::string expression = _cb->ExpressionStr();

    if (_cb->CondBlockType() == Ziz::Ifndef)
        expression = " ! " + expression;

    return _expression = strdup(expression.c_str());
}

ZizCondBlockPtr::~ZizCondBlockPtr() {
    if (_expression != NULL)
        free(_expression);
}

BlockCloud::BlockCloud() : _constraints(NULL) {}
BlockCloud::BlockCloud(Ziz::ConditionalBlock *cb) : _constraints(NULL) {
    try {
        this->scanBlocks(cb);
    } catch (std::runtime_error &e) {
        std::cerr << "failed to parse a Blockcontainer with "
                  << cb->size() << " Blocks" << std::endl
                  << e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
}

BlockCloud::~BlockCloud() {
    if (_constraints != NULL)
        delete _constraints;
}

const std::string& BlockCloud::getConstraints() const {
    StringJoiner sj;

    if (_constraints)
        return *_constraints;

    for (index i = 0; i < size(); i++) {
        StringJoiner pc;
        pc.push_back(parent(i));
        std::string exp = expression(i);
        pc.push_back(exp);
        pc.push_back(noPredecessor(i));

        std::string bn = getBlockName(i);
        sj.push_back("( " + bn + " <-> " + pc.join(" && ") + " )");

        std::string type = ( (exp.find("&&") != std::string::npos) || exp.find("||") != std::string::npos)  ? ":logic" : ":symbolic"; //should we add here a third type "intentional" for if 0 and if 1 ???
        std::stringstream ss;
        ss << (*this)[i]._cb->Start() << ":" << (*this)[i]._cb->End() << type;
        this->positions.insert(std::pair<std::string,std::string>(getBlockName(i), ss.str()));
        std::string position = ss.str();
        this->positions[bn] = position;
    }
    _constraints = new std::string(sj.join("\n&& "));

    return *_constraints;
}

int BlockCloud::scanBlocks(Ziz::BlockContainer *bc) {
    Ziz::ConditionalBlock::iterator i;
    int count = 0;

    if (!bc && bc->size() == 0)
        return 0;

    for (i = bc->begin(); i != bc->end(); i++) {
        Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
        if (cb) {
            count++;
#ifdef DEBUG
            std::cout << cb->Start() << ": Nested in block " << cb->Id()
                      << " (" << this << ")" << std::endl;
#endif
            this->push_back(ZizCondBlockPtr(cb));
            count += this->scanBlocks(cb);
        }
    }

    return count;
}

int BlockCloud::bId(index n) const {
    return item(n).getId();
}


BlockCloud::index BlockCloud::search(int id) const {

    std::stringstream ss;
    for (index i = 0; i < size(); i++)
        if (id == bId(i))
            return i;
    ss << "id not found: " << id;
    throw std::runtime_error(ss.str());
}

BlockCloud::index BlockCloud::search(std::string idstring) const {
    int id;
    idstring = idstring.substr(1);
    std::stringstream ss(idstring);
    ss >> id;
    return search(id);
}

const ZizCondBlockPtr &BlockCloud::item(index n) const {
    return (*this)[n];
}


std::string BlockCloud::getBlockName(index n) const {
    std::stringstream ss;
    ss << "B" << bId(n);
    return ss.str();
}

std::string BlockCloud::parent(index n) const {
    std::stringstream ss;

    const Ziz::ConditionalBlock *b = item(n).Block();
    if((b = b->ParentCondBlock())) {
        ss << " ( " << getBlockName(search(b->Id())) << " ) ";
    }
    return ss.str();
}

std::string BlockCloud::expression(index n) const {
    return item(n).expression();
}

std::string BlockCloud::noPredecessor(index n) const {
    std::stringstream ss;

    const Ziz::ConditionalBlock *b = item(n).Block();
    StringJoiner sj;

    while(b->PrevSibling()) {
        sj.push_back(getBlockName(search(b->PrevSibling()->Id())));
        b = b->PrevSibling();
    };

    if (sj.size() > 0)
        return  "( ! (" + sj.join(" || ") + ") ) ";
    else
        return "";
}


CloudContainer::CloudContainer(const char *filename)
    : _zfile(NULL), _fail(false), _constraints(NULL), _filename(filename) {
    Ziz::Parser parser;

    try {
        _zfile = parser.Parse(filename);
    } catch(Ziz::ZizException& e) {
        std::cerr << "caught ZizException: " << e.what() << std::endl;
        _fail = true;
    } catch(...) {
        std::cerr << "Failed to parse '" << filename << "'" <<std::endl;
        _fail = true;
    }
}

CloudContainer::~CloudContainer() {
    if(_zfile)
        delete _zfile;
    if(_constraints != NULL)
        delete _constraints;
}

ParentMap CloudContainer::getParents() const {
    std::map<std::string, std::string> ret;
    for (CloudList::const_iterator c = this->begin(); c != this->end(); c++) {
        for (unsigned int i = 0; i < c->size(); i++) {
            std::string papa(c->parent(i));
            if (!papa.empty()) {
                std::string me(c->getBlockName(i));
                ret.insert(std::pair<std::string, std::string>(me, papa));
            }
        }
    }
    return ret;
}

const std::string& CloudContainer::getConstraints() {
    StringJoiner sj;

    if (!_zfile)
        return *_constraints;

    if (_constraints)
        return *_constraints;

    if (size() == 0) { // we didn't scan for conditional blocks yet
        Ziz::BlockContainer::iterator i;
        for (i = _zfile->begin(); i != _zfile->end(); i++) {
            Ziz::ConditionalBlock *cb = dynamic_cast<Ziz::ConditionalBlock*>(*i);
            if(cb) {

                // a cloud consists of top level conditional blocks but include all
                // sister blocks!

                if (size() == 0 || (cb->PrevSibling() == NULL)) // start new cloud if no prev sibling
                    this->push_back(BlockCloud());

#ifdef DEBUG
                std::cout << cb->Start() << ": toplevel block " << cb->Id()
                          << " (" << &(this->back()) << ")"
                          << std::endl;
#endif

                this->back().push_back(ZizCondBlockPtr(cb));
                this->back().scanBlocks(cb);
            }
        }
    }

    unsigned int cloudno = 0;

    rewriteAllExpressions();

    for (CloudList::iterator c = this->begin(); c != this->end(); c++) {
        try {
            sj.push_back(c->getConstraints());
            cloudno++;
        } catch (std::runtime_error &e) {
            std::cerr << "failed to process cloud no: "  << cloudno << std::endl
                      << e.what() << std::endl;
        }
    }

    for(std::set<std::string>::iterator i = rw_constraints.begin(); i != rw_constraints.end(); i++)
        sj.push_back("( "+ *i + " )");

    sj.push_back("B00");
    _constraints = new std::string(sj.join("\n&& "));

    return *_constraints;
}

using namespace Ziz;

void CloudContainer::rewriteAllExpressions()
{
    CloudContainer *cc = this;
    for(std::deque<BlockCloud>::iterator bci = cc->begin(); bci != cc->end(); bci++) {
        for(std::deque<ZizCondBlockPtr>::iterator zi = (*bci).begin(); zi != (*bci).end(); zi++) {
            cc->rewriteExpression((*zi));
        }
    }
#ifdef DEBUG
    for(std::set<std::string>::iterator i = rw_constraints.begin(); i != rw_constraints.end(); i++)
        std::cout << *i << std::endl;
#endif
}

int getLineFromPos(position_type pos)
{
     std::stringstream ss;
     ss << pos;
     std::string raw = ss.str();
     size_t p = std::string::npos;
     while ( (p = raw.find(":")) != std::string::npos) {
       raw.replace(p,1,1,' ');
     }
     std::string filename, line;
     std::stringstream sss(raw);
     sss >> filename;
     sss >> line;
     return atoi(line.c_str());
}


bool defineInBetween(int current_pos, int last_pos, Ziz::Defines& defs, std::string flag)
{
    std::list<Define*> list = defs[flag];
    for(std::list<Define*>::iterator it = list.begin(); it != list.end(); it++) {
        int define_pos = getLineFromPos((*it)->_position);
        if ( (define_pos > last_pos) && (define_pos < current_pos) )
            return true;
    }

    return false;
}

int CloudContainer::getLastDefinePos(std::string flag, int limit)
{
    Ziz::Defines defines_map = _zfile->getDefinesMap();
    int last = 0;
    for(std::list<Define*>::iterator it = defines_map[flag].begin(); it != defines_map[flag].end(); it++) {
        int cur = getLineFromPos((*it)->_position);
        if (cur > limit)
            return last;

        last = cur;
    }
    return last;
}

bool CloudContainer::isLastDefineAtLevel(Define* def, int limit)
{
    int define_pos = getLineFromPos(def->_position);
    int last = getLastDefinePosFromDepth(def->_flag,limit,def->_block);
    return define_pos == last;
}

int CloudContainer::getLastDefinePosFromDepth(std::string flag, int limit, Ziz::ConditionalBlock* cb)
{
    Ziz::Defines defines_map = _zfile->getDefinesMap();
    int last = 0;
    for(std::list<Define*>::iterator it = defines_map[flag].begin(); it != defines_map[flag].end(); it++) {
        int cur_pos = getLineFromPos((*it)->_position);
        if (cur_pos > limit)
            return last;

        if (cb == (*it)->_block)
            last = cur_pos;
    }
    return last;
}

void print_define(Define* d) {
    std::cout << "define._id: " << d->_id << std::endl;
    std::cout << "define._flag: " << d->_flag << std::endl;
    std::cout << "define._position: " << d->_position << std::endl;
    std::cout << "define._block: " << d->_block << std::endl;
    std::cout << "define._define " << d->_define << std::endl;
}

std::string block_name(int i)
{
    std::stringstream ss;
    ss << "B" << i;
    return ss.str();
}

std::string CloudContainer::rewriteExpression(ZizCondBlockPtr& block)
{
    std::map<std::string,std::string> replace;
    static std::map<std::string,std::string> last_change;
    std::string exp = block.Block()->ExpressionStr();
    static std::map<std::string,std::list<int> > new_vars;
    Ziz::Defines defines_map = _zfile->getDefinesMap();
    static const boost::regex free_item_regexp("[a-zA-Z0-9\\-_]+", boost::regex::extended);
    static const boost::regex non_items("B[0-9]+", boost::regex::extended);
    std::stringstream ss(exp);
    std::string item;
    std::stringstream result;

    while (ss >> item) {
        if (replace.find(item) != replace.end()) { // item already processed for the current exp
            continue;
        }
        if (boost::regex_match(item.begin(), item.end(), non_items) || item.compare("defined") == 0) {
            continue; // not a relevant item
        }

        if (defines_map.count(item) == 0) {
            continue; //no define or undef for this item
        }


        int current_pos = getLineFromPos(block.Block()->Start());
        if (getLastDefinePos(item,current_pos)) { // we have a relevant define for this flag

            bool new_constraint = false;

            std::string new_item;
            if (new_vars.find(item) == new_vars.end()  ) { // this is the first rewriting of item taking place
                new_vars[item].push_back(current_pos);
                new_item = item+".";
                last_change[item] = new_item;
                replace[item] = new_item;
                new_constraint = true;
            } else {
                int last_pos = *(--new_vars[item].end()); // last position where a rewriting of item took place
                if (defineInBetween(current_pos, last_pos, defines_map, item)) {
                    new_vars[item].push_back(current_pos);
                    int c = new_vars[item].size();
                    std::string append = "";
                    for(int i=0; i<c; i++) {
                        append += "." ;
                    }
                    new_item = item+append;
                    last_change[item] = new_item;
                    replace[item] = new_item;
                    new_constraint = true;
                } else {
                    replace[item] = last_change[item]; // no need for a new variable just use the last definition
                }
            }
            // constraints for the new item
            if (new_constraint) {
                std::stack<std::string> successors;
                std::stack<std::string> depends;
                bool first = true;
                for(std::list<Define*>::iterator it = defines_map[item].begin(); it != defines_map[item].end(); it++) { // iteration through all defines of one specific flag

                    int cur_define_pos = getLineFromPos((*it)->_position);

                    if ( cur_define_pos > current_pos )
                        break;

                    if (!isLastDefineAtLevel((*it),current_pos))
                        continue;


                    bool global = ((*it)->_block == 0); // define in the global scope
                    std::string main_define_block = "B00";
                    if (!global) {
                        main_define_block = block_name((*it)->_block->Id());
                        depends.push(block_name((*it)->_block->Id())); //THINK HERE
                    } else {
		        depends.push(main_define_block);
		    }

                    std::list<Define*>::iterator from = it;
                    int ignore_rw_c = false; //ignore rw constraint
                    if (++from != defines_map[item].end()) {
                        for(std::list<Define*>::iterator jt = from; jt != defines_map[item].end(); jt++) {
                            int suc_pos = getLineFromPos((*jt)->_position);

                            if (suc_pos > current_pos)
                                break;

                            if (!isLastDefineAtLevel((*jt),current_pos))
                                continue;

                            if ((*jt)->_block) { // not global
                                std::string succ_define_block = block_name((*jt)->_block->Id());
                                successors.push(succ_define_block);
                                depends.push(succ_define_block);
                            } else {
                                successors.push("B00");
                                depends.push("B00");
                                ignore_rw_c = true; //if this define depends on the non existence of the global scope, we don't need this constraint
                            }
                        }
                    }
                    //print_define(*it);
                    //std::cout << "--" << std::endl;
                    StringJoiner sj;
                    std::string cur_item = replace[item];
                    while(!successors.empty()) {
                        sj.push_back(successors.top());
                        successors.pop();
                    }
                    std::string suc = sj.join(" | ");
                    std::stringstream nc;
                    std::string right_side = ((*it)->_define) ? cur_item : "!"+cur_item;
                    if (suc.empty()) {
                        //if (!global)
                        nc << main_define_block << " -> " << right_side;
                    } else {
                        //if (!global)
                        nc << "( " << main_define_block << " && ";
                        //else
                        //  nc << "( ";
                        nc << "!( " << suc << " ) )" << " -> " ;
                        nc << right_side;
                    }
                    rw_constraints.insert(nc.str());
                    StringJoiner dj;
                    while(!depends.empty()) {
                        dj.push_back(depends.top());
                        depends.pop();
                    }
                    if (first) {
                        std::string deps = dj.join(" | ");
                        std::stringstream s1,s2;
                        std::string prev = replace[item].substr(0,replace[item].size()-1);
                        s1 << " ( (" << prev << ")";
                        if (deps.compare("") != 0)
                            s1 << " && !(" << deps << ") )";
                        else
                            s1 << " ) ";
                        s1 << " -> " << replace[item];
                        rw_constraints.insert(s1.str());

                        s2 << " ( (" << replace[item]<< ") ";
                        if (deps.compare("") != 0)
                            s2 << " && !(" << deps << ") )";
                        else
                            s2 << " ) ";
                        s2 << " -> " << prev;
                        rw_constraints.insert(s2.str());
                        first = false;
                    }
                }
            }
        }
    }
    for(std::map<std::string,std::string>::iterator it = replace.begin(); it != replace.end(); it++) {
        std::string alt = (*it).first;
        std::string neu = (*it).second;
        boost::regex alt_regex(alt, boost::regex::sed);
        std::string result = boost::regex_replace(exp,alt_regex,neu);
        exp = result;
    }

    if (block.Block()->CondBlockType() == Ziz::Ifndef)
        exp = " ! " + exp;

    free(block._expression);
    block._expression = strdup(exp.c_str());
    
    return exp;
}
