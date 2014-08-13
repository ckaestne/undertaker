/*
 *   boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013-2014 Stefan Hengelein <stefan.hengelein@fau.de>
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
#include "exceptions/BoolExpParserException.h"
#include "BoolExpGC.h"
#include "BoolExpSimplifier.h"
#include "BoolExpStringBuilder.h"
#include "BoolExpLexer.h"

#include <typeinfo> // for typeid()
#include <sstream>


/************************************************************************/
/* accept Methods for BoolExp and subclasses. Needed by BoolExpVisitor  */
/************************************************************************/

void kconfig::BoolExp::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpAny::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpAnd::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpOr::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpImpl::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpEq::accept(kconfig::BoolVisitor *visitor) {
    void *l = nullptr, *r = nullptr;
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
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpNot::accept(kconfig::BoolVisitor *visitor) {
    void *r = nullptr;
    if (this->right) {
        if (visitor->isVisited(right)) {
            r = visitor->visited[right];
        } else {
            this->right->accept(visitor);
            r = visitor->result;
        }
    }
    visitor->left = nullptr;
    visitor->right = r;
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpConst::accept(kconfig::BoolVisitor *visitor) {
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

kconfig::BoolExpConst *kconfig::BoolExpConst::getInstance(bool val) {
    return new kconfig::BoolExpConst(val);
}

void kconfig::BoolExpVar::accept(kconfig::BoolVisitor *visitor) {
    visitor->result = nullptr;
    visitor->visit(this);
    visitor->visited[this] = visitor->result;
}

void kconfig::BoolExpCall::accept(kconfig::BoolVisitor *visitor) {
    for (const auto &bool_exp : *this->param)  // BoolExp *
        bool_exp->accept(visitor);
    visitor->visit(this);
}

/************************************************************************/
/* Equals methods                                                       */
/************************************************************************/

bool kconfig::BoolExp::equals(const BoolExp *other) const {
    if (other == nullptr || typeid(*other) != typeid(*this)) {
        return false;
    } else {
        return ((this->left == other->left || this->left->equals(other->left))
                && (this->right == other->right || this->right->equals(other->right)));
    }
}

bool kconfig::BoolExpCall::equals(const BoolExp *other) const {
    const BoolExpCall *otherc = dynamic_cast<const BoolExpCall *>(other);
    if (otherc == nullptr || this->name != otherc->name
        || this->param->size() != otherc->param->size()) {
        return false;
    }
    auto ito = otherc->param->begin();  // BoolExp *
    for (const auto &entry : *param) {  // BoolExp *
        if (!entry->equals(*ito))
            return false;
        ++ito;
    }
    return true;
}

bool kconfig::BoolExpVar::equals(const BoolExp *other) const {
    const BoolExpVar *otherv = dynamic_cast<const BoolExpVar *>(other);
    return otherv != nullptr && this->name == otherv->name;
}

bool kconfig::BoolExpConst::equals(const BoolExp *other) const {
    const BoolExpConst *otherc = dynamic_cast<const BoolExpConst *>(other);
    return otherc != nullptr && this->value == otherc->value;
}

bool kconfig::BoolExpAny::equals(const BoolExp *other) const {
    const BoolExpAny *otherc = dynamic_cast<const BoolExpAny *>(other);
    return otherc != nullptr && this->name == otherc->name
           && ((this->left == otherc->left || this->left->equals(otherc->left))
               && (this->right == otherc->right || this->right->equals(otherc->right)));
}

/************************************************************************/
/* BoolExp baseclass methods                                            */
/************************************************************************/

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
    BoolExpLexer lexer(&ins, nullptr);

    BoolExpParser parser(&result, &lexer);
    try {
        if (parser.parse() == 0) {
            return result;
        }
    } catch (BoolExpParserException *e) {
        return nullptr;
    }
    return nullptr;
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

/************************************************************************/
/* Operators                                                            */
/************************************************************************/

std::ostream &kconfig::operator<<(std::ostream &s, kconfig::BoolExp &exp) {
    s << exp.str();
    return s;
}

kconfig::BoolExp &kconfig::operator&&(kconfig::BoolExp &l, kconfig::BoolExp &r) {
    BoolExpConst *right = dynamic_cast<BoolExpConst *>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst *>(&l);

    if (left != nullptr || right != nullptr) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == true) {
            return var;
        } else {
            return c;  // false
        }
    }
    return *B_AND(&l, &r);
}

kconfig::BoolExp &kconfig::operator||(kconfig::BoolExp &l, kconfig::BoolExp &r) {
    BoolExpConst *right = dynamic_cast<BoolExpConst *>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst *>(&l);

    if (left != nullptr || right != nullptr) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == false) {
            return var;
        } else {
            return c;  // true
        }
    }
    return *B_OR(&l, &r);
}

kconfig::BoolExp &kconfig::operator!(kconfig::BoolExp & l) {
    BoolExpConst *lConst = dynamic_cast<BoolExpConst *>(&l);

    if (lConst) {
        bool newval = !(lConst->value);
        return *B_CONST(newval);
    }
    return *B_NOT(&l);
}
