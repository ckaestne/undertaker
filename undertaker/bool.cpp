/*
 *   satyr - compiles KConfig files to boolean formulas
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bool.h"
#include "BoolExpParserException.h"
#include <iostream>
#include <sstream>

using namespace kconfig;
using namespace std;

void kconfig::BoolExp::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpAny::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpAnd::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpOr::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpImpl::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpEq::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpNot::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

void kconfig::BoolExpConst::accept(kconfig::BoolVisitor *visitor)
{
    if (this->left) {
        this->left->accept(visitor);
    }
    if (this->right) {
        this->right->accept(visitor);
    }
    visitor->visit(this);
}

kconfig::BoolExpConst *kconfig::BoolExpConst::getInstance(bool val)
{
    //TODO memory management
    //static BoolExpConst *no = new  kconfig::BoolExpConst(false);
    //static BoolExpConst *yes = new kconfig::BoolExpConst(true);

    //return val ? yes : no;
    return new kconfig::BoolExpConst(val);
}

void kconfig::BoolExpVar::accept(kconfig::BoolVisitor *visitor)
{
    visitor->visit(this);
}

void kconfig::BoolExpCall::accept(kconfig::BoolVisitor *visitor)
{
	std::list<BoolExp *>::const_iterator it;
    for (it = this->param->begin(); it!= this->param->end(); it++){
        (*it)->accept(visitor);
    }
    visitor->visit(this);
}

/*simplification rules*/
BoolExp *kconfig::BoolExpNot::simplify(bool useAsumtions)
{
    BoolExp *node = this->right->simplify(useAsumtions);
    BoolExpConst *constant = dynamic_cast<BoolExpConst*>(node);
    if (constant) {
        bool v = !constant->value;

        BoolExp *ret = B_CONST(v);
        delete constant;
        return ret;
    }

    BoolExpNot *notExpr = dynamic_cast<BoolExpNot*>(node);
    if (notExpr) {
        BoolExp *x = notExpr->right;
        notExpr->right = 0;
        delete notExpr;
        return x;
    }
    return new BoolExpNot(node);
}

BoolExp *kconfig::BoolExpAnd::simplify(bool useAsumtions)
{
    BoolExp *sr = this->right->simplify(useAsumtions);
    BoolExp *sl = this->left->simplify(useAsumtions);
    BoolExpConst *right = dynamic_cast<BoolExpConst*>(sr);
    BoolExpConst *left = dynamic_cast<BoolExpConst*>(sl);

    if (left != 0 || right != 0) {
        BoolExpConst *c = left ? left : right;
        BoolExp *var = left ? sr : sl;
        if (c->value == true) {
            delete c;
            return var;
        }
        else {
            delete var;
            return c;            //0
        }
    }
    {
        // X && X
        BoolExpVar *right = dynamic_cast<BoolExpVar*>(sr);
        BoolExpVar *left = dynamic_cast<BoolExpVar*>(sl);
        if (left != 0 && right != 0) {
            if (right->str() == left->str()) {
                return left;
            }
        }
    }

    return new BoolExpAnd(sl, sr);
}

BoolExp *kconfig::BoolExpOr::simplify(bool useAsumtions)
{
    BoolExp *sr = this->right->simplify(useAsumtions);
    BoolExp *sl = this->left->simplify(useAsumtions);
    {
        BoolExpConst *right = dynamic_cast<BoolExpConst*>(sr);
        BoolExpConst *left = dynamic_cast<BoolExpConst*>(sl);

        // x || 0; X||1
        if (left != 0 || right != 0) {
            BoolExpConst *c = left ? left : right;
            BoolExp *var = left ? sr : sl;
            if (c->value == false) {
                delete c;
                return var;
            }
            else {
                delete var;
                return c;        //1
            }
        }
    }
    {
        // X || !X
        BoolExpVar *right = dynamic_cast<BoolExpVar*>(sr);
        BoolExpVar *left = dynamic_cast<BoolExpVar*>(sl);
        if (left != 0 || right != 0) {
            BoolExpVar *var = left ? left : right;
            BoolExp *other = left ? sr : sl;
            BoolExpNot *inverse = dynamic_cast<BoolExpNot*>(other);
            //TODO replace with equal()
            if (inverse && inverse->right->str() == var->str()) {
                delete right;
                delete left;
                return B_CONST(true);
            }
        }
    }
    return new BoolExpOr(sl, sr);
}

BoolExp *kconfig::BoolExpImpl::simplify(bool useAsumtions)
{
    BoolExp *sr = this->right->simplify(useAsumtions);
    BoolExp *sl = this->left->simplify(useAsumtions);
    return new BoolExpImpl(sl, sr);
}

BoolExp *kconfig::BoolExpEq::simplify(bool useAsumtions)
{
    BoolExp *sr = this->right->simplify(useAsumtions);
    BoolExp *sl = this->left->simplify(useAsumtions);
    return new BoolExpEq(sl, sr);
}

std::ostream& kconfig::operator<<(std::ostream &s, kconfig::BoolExp &exp)
{
    s << exp.str();
    return s;
}

kconfig::BoolExp & kconfig::operator &&(kconfig::BoolExp &l, kconfig::BoolExp &r)
{
    kconfig::BoolExp *e;

    BoolExpConst *right = dynamic_cast<BoolExpConst*>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst*>(&l);

    if (left != 0 || right != 0) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == true) {
            return var;
        }
        else {
            return c;            //false
        }
    }
    e = B_AND(&l, &r);
    return *e;
}

kconfig::BoolExp & kconfig::operator ||(kconfig::BoolExp &l, kconfig::BoolExp &r)
{
    BoolExpConst *right = dynamic_cast<BoolExpConst*>(&r);
    BoolExpConst *left = dynamic_cast<BoolExpConst*>(&l);

    if (left != 0 || right != 0) {
        BoolExpConst &c = left ? *left : *right;
        BoolExp &var = left ? r : l;
        if (c.value == false) {
            return var;
        }
        else {
            return c;            //true
        }
    }

    kconfig::BoolExp *e = B_OR(&l, &r);
    return *e;
}

kconfig::BoolExp & kconfig::operator !(kconfig::BoolExp &l)
{

    BoolExp *e;
    BoolExpConst *lConst =  dynamic_cast<BoolExpConst *>(&l);
    if (lConst) {
        bool newval = !(lConst->value);
        return *B_CONST(newval);
    }

    e = B_NOT(&l);
    return *e;
}

kconfig::BoolExp *kconfig::BoolExp::parseString(std::string s)
{
    BoolExp *result;
    stringstream ins(s);
    BoolExpLexer lexer(&ins,0);

    BoolExpParser parser(&result, &lexer);
    try
    {
        if (parser.parse() == 0) {
            return result;
        }
    }
    catch (BoolExpParserException *e) {
        return 0;
    }
    return 0;
}
