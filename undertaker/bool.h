/*
 *   boolframwork - boolean framework for undertaker and satyr
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

#ifndef KCONFIG_BOOL_H
#define KCONFIG_BOOL_H

#include <string>
#include <list>
#include <ostream>
#include <iostream>
#include "BoolExpLexer.h"

#define KCONFIG_BOOL_SHORTCUTS

#ifdef KCONFIG_BOOL_SHORTCUTS

#define B_AND new kconfig::BoolExpAnd
#define B_OR new kconfig::BoolExpOr
#define B_NOT new kconfig::BoolExpNot
#define B_VAR new kconfig::BoolExpVar
#define B_CONST(v) (kconfig::BoolExpConst::getInstance(v))
#endif

namespace kconfig
{

    //class BoolExpParser;
    class BoolExpLexer;
    class BoolVisitor;

    enum BoolType
    {
        NONE,
        CONST,
        VAR,
        NOT,
        AND,
        OR
    };

    enum TristateRelation
    {
        rel_yes, rel_mod, rel_pres, rel_helper, rel_meta
    };

    const std::string TristateRelationNames[] = { "", "_MODULE", "_PRESENT", "", "_META" };

    class BoolExp
    {
        protected:
            int prio;
            int linkCount;
            bool persistent;
        public:
            BoolExp *left;
            BoolExp *right;
            BoolType type;
            int CNFVar;
        public:
            BoolExp() {
                CNFVar = 0;
                persistent = false;
                left = 0;
                right = 0;
            }
            virtual std::string bstr(int prio) {
                return (prio <= this->prio) ? str() : "(" + str() + ")";
            }
            virtual std::string str(void) {
                return "ERROR";
            }
            virtual BoolExp *simplify(bool) {
                return 0;
            }
            unsigned int nodeCount() {
                unsigned int nc = 1;
                if (right) {
                    nc += right->nodeCount();
                }
                if (left) {
                    nc += left->nodeCount();
                }
                return nc;
            }
            virtual void accept(BoolVisitor *visitor);
            /*TODO use garbage pools*/
            virtual ~BoolExp() {
                if (right && !right->persistent) {
                    delete right;
                }
                if (left && !left->persistent) {
                    delete left;
                }
            }
            static BoolExp *parseString(std::string);
    };
    std::ostream& operator<<(std::ostream &s, BoolExp &exp);

    class BoolExpAnd : public BoolExp
    {
        public:
            BoolExpAnd(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
                prio = 50;
                type = AND;
            }
            virtual void accept(BoolVisitor *visitor);
            virtual std::string str(void) {
                return left->bstr(prio) + " && " + right->bstr(prio);
            }
            virtual BoolExp *simplify(bool useAsumtions = false);

    };

    class BoolExpOr : public BoolExp
    {
        public:
            BoolExpOr(BoolExp *el, BoolExp *er): BoolExp() {
                right = er;
                left = el;
                prio = 30;
                type = OR;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return left->bstr(prio) + " || " + right->bstr(prio);
            }

            virtual BoolExp *simplify(bool useAsumtions = false);
    };

    class BoolExpAny : public BoolExp
    {
        private:
            std::string name;
        public:
            BoolExpAny(std::string name, BoolExp *el, BoolExp *er):BoolExp() {
                right = er;
                left = el;
                prio = 60;
                type = OR;
                this->name = name;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return left->bstr(prio) + " " + name + " "+ right->bstr(prio);
            }

            virtual BoolExp *simplify(bool) {
                return 0;
            }
    };

    class BoolExpImpl : public BoolExp
    {
        public:
            BoolExpImpl(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
                prio = 20;
                type = OR;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return left->bstr(prio) + " -> " + right->bstr(prio);
            }

            virtual BoolExp *simplify(bool useAsumtions = false);
    };
    class BoolExpEq : public BoolExp
    {
        public:
            BoolExpEq(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
                prio = 10;
                type = OR;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return left->bstr(prio) + " <-> " + right->bstr(prio);
            }

            virtual BoolExp *simplify(bool useAsumtions = false);
    };

    class BoolExpCall : public BoolExp
    {
        private:
            std::string name;
            std::list<BoolExp *> *param;
        public:
            BoolExpCall(std::string name, std::list<BoolExp *> *param) :BoolExp() {
                this->name = name;
                this->prio = 90;
                this->param = param;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                bool first = true;
                std::string paramstring("");
                for (std::list<BoolExp *>::const_iterator it = param->begin(); it !=param->end(); it++) {
                    paramstring += first ? "" : ", ";
                    paramstring += (*it)->str();
                    first = false;
                }
                return name+ " (" + paramstring + ")";
            }

            virtual BoolExp *simplify(bool) {
                return 0;
            }
            virtual ~BoolExpCall() {
                for (std::list<BoolExp *>::const_iterator it = param->begin(); it !=param->end(); it++) {
                    delete *it;
                }
                delete param;
            }
            std::string getName(void){
            	return name;
            }
    };

    class BoolExpNot : public BoolExp
    {
        public:
            BoolExpNot(BoolExp *e) :BoolExp() {
                right = e;
                left = 0;
                prio = 70;
                type = NOT;
                this->left = 0;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return "!" + right->bstr(prio);
            }

            virtual BoolExp *simplify(bool useAsumtions = false);

    };

    class BoolExpConst : public BoolExp
    {
        public:
            bool value;
        private:
            BoolExpConst(bool val):BoolExp() {
                value = val;
                prio = 90;
                type = CONST;
                persistent = false;
                this->left = 0;
                this->right = 0;
            }
        public:
            static BoolExpConst *getInstance(bool val);
            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return value ? "1" : "0";
            }

            virtual BoolExp *simplify(bool) {
                return new BoolExpConst(*this);
            }
    };

    class BoolExpVar : public BoolExp
    {
        std::string name;
        TristateRelation rel;

        public:
            BoolExpVar(std::string name, bool addPrefix=true):BoolExp() {
                this->rel = rel_helper;
                this->name = addPrefix ? "CONFIG_" + name: name;
                this->prio = 90;
                this->left = 0;
                this->right = 0;
                this->type = VAR;
            }

            virtual void accept(BoolVisitor *visitor);

            virtual std::string str(void) {
                return name;
            }

            virtual BoolExp *simplify(bool /* useAsumtions = false*/) {
                return new BoolExpVar(*this);
            }

    };

    BoolExp &operator &&(BoolExp &l, BoolExp &r);
    BoolExp &operator ||(BoolExp &l, BoolExp &r);
    BoolExp &operator !(BoolExp &l);

    class BoolVisitor
    {
        friend class BoolExpVar;
        friend class BoolExpNot;
        friend class BoolExpConst;
        friend class BoolExpOr;
        friend class BoolExpAnd;
        friend class BoolExpImpl;
        friend class BoolExpEq;
        friend class BoolExp;
        friend class BoolExpCall;
        friend class BoolExpAny;
        protected:
            virtual void visit(BoolExp *e) = 0;
            virtual void visit(BoolExpAnd *e) = 0;
            virtual void visit(BoolExpOr *e) = 0;
            virtual void visit(BoolExpNot *e) = 0;
            virtual void visit(BoolExpConst *e) = 0;
            virtual void visit(BoolExpVar *e) = 0;
            virtual void visit(BoolExpImpl *e) = 0;
            virtual void visit(BoolExpEq *e) = 0;
            virtual void visit(BoolExpCall *e) = 0;
            virtual void visit(BoolExpAny *e) = 0;

    };

};
#endif
