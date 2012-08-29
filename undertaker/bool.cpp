/*
 * boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "bool.h"
#include "BoolExpParserException.h"
#include "BoolExpGC.h"
#include "BoolExpSimplifier.h"
#include "BoolExpStringBuilder.h"

#include <sstream>
#include <typeinfo> // for typeid()

/* accept Methods for BoolExp and subclasses. Needed by BoolExpVisitor */
void kconfig::BoolExp::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpAny::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpAnd::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpOr::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpImpl::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpEq::accept(kconfig::BoolVisitor *visitor) {
    void *l = NULL, *r = NULL;
    if (this->left) {
        if (visitor->isVisited(left)) {
            l = visitor->visited[left];
        } else {
            this->left->accept(visitor);
            l = visitor->result;
        }
    }
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = l;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpNot::accept(kconfig::BoolVisitor *visitor) {
    void *r = NULL;
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = NULL;
    visitor->right = r;
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpConst::accept(kconfig::BoolVisitor *visitor) {
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

kconfig::BoolExpConst *kconfig::BoolExpConst::getInstance(bool val) {
    return new kconfig::BoolExpConst(val);
}

void kconfig::BoolExpVar::accept(kconfig::BoolVisitor *visitor) {
    visitor->result = NULL;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpCall::accept(kconfig::BoolVisitor *visitor) {
    std::list<BoolExp *>::const_iterator it;
    for (it = this->param->begin(); it!= this->param->end(); it++){
        (*it)->accept(visitor);
    }
    visitor->visit(this);
}
/*
 * END: accept methods
 */

/* Equal methods*/

bool kconfig::BoolExp::equals(const BoolExp *other) const {
    if (other == NULL || typeid(*other) != typeid(*this)) {
        return false;
    } else {
        return ( (this->left == other->left || this->left->equals(other->left))
              && (this->right == other->right || this->right->equals(other->right)) );
    }
}

bool kconfig::BoolExpCall::equals(const BoolExp *other) const {
    const BoolExpCall *otherc = dynamic_cast<const BoolExpCall *>(other);
    if ( otherc == NULL
           || this->name != otherc->name
           || this->param->size() != otherc->param->size()) {
        return false;
    }
    std::list<BoolExp *>::const_iterator itt = param->begin();
    std::list<BoolExp *>::const_iterator ito  = otherc->param->begin();
    for ( ; itt !=param->end(); itt++, ito++) {
        if ( ! (*itt)->equals(*ito)) {
            return false;
        }
    }
    return true;
}

bool kconfig::BoolExpVar::equals(const BoolExp *other) const {
    const BoolExpVar *otherv = dynamic_cast<const BoolExpVar *>(other);
    return otherv != NULL && this->name == otherv->name;
}

bool kconfig::BoolExpConst::equals(const BoolExp *other) const {
    const BoolExpConst *otherc = dynamic_cast<const BoolExpConst *>(other);
    return otherc != NULL && this->value == otherc->value;
}

bool kconfig::BoolExpAny::equals(const BoolExp *other) const {
    const BoolExpAny *otherc = dynamic_cast<const BoolExpAny *>(other);
    return otherc != 0 && this->name == otherc->name
         && ( (this->left == otherc->left || this->left->equals(otherc->left))
         && (this->right == otherc->right || this->right->equals(otherc->right)) );
}

/* END: Equal methods*/

std::ostream& kconfig::operator<<(std::ostream &s, kconfig::BoolExp &exp) {
    s << exp.str();
    return s;
}

kconfig::BoolExp & kconfig::operator &&(kconfig::BoolExp &l, kconfig::BoolExp &r) {
    kconfig::BoolExp *e;

    BoolExpConst *right = dynamic_cast<BoolExpConst*>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst*>(&l);

    if (left != NULL || right != NULL) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == true) {
            return var;
        } else {
            return c;            //false
        }
    }
    e = B_AND(&l, &r);
    return *e;
}

kconfig::BoolExp & kconfig::operator ||(kconfig::BoolExp &l, kconfig::BoolExp &r) {
    BoolExpConst *right = dynamic_cast<BoolExpConst*>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst*>(&l);

    if (left != NULL || right != NULL) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == false) {
            return var;
        } else {
            return c;            //true
        }
    }
    kconfig::BoolExp *e = B_OR(&l, &r);
    return *e;
}

kconfig::BoolExp & kconfig::operator !(kconfig::BoolExp &l) {
    BoolExp *e;
    BoolExpConst *lConst =  dynamic_cast<BoolExpConst *>(&l);

    if (lConst) {
        bool newval = !(lConst->value);
        return *B_CONST(newval);
    }
    e = B_NOT(&l);
    return *e;
}

kconfig::BoolExp::~BoolExp() {
    if (!gcMarked) {
        BoolExpGC gc;
        gc.trash(this);
        gc.sweep(this);
    }
}

kconfig::BoolExp *kconfig::BoolExp::parseString(std::string s) {
    BoolExp *result;
    std::stringstream ins(s);
    BoolExpLexer lexer(&ins, NULL);

    BoolExpParser parser(&result, &lexer);
    try {
        if (parser.parse() == 0) {
            return result;
        }
    } catch (BoolExpParserException *e) {
        return NULL;
    }
    return NULL;
}

std::string kconfig::BoolExp::str(void) {
    BoolExpStringBuilder sb;
    this->accept(&sb);
    return sb.str();
}

kconfig::BoolExp *kconfig::BoolExp::simplify() {
    BoolExpSimplifier sim;
    this->accept(&sim);
    return sim.getResult();
}
