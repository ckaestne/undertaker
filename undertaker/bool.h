// -*- mode: c++ -*-
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

#ifndef KCONFIG_BOOL_H
#define KCONFIG_BOOL_H

#include <string>
#include <list>
#include <ostream>
#include "BoolExpLexer.h"

#define B_AND new kconfig::BoolExpAnd
#define B_OR new kconfig::BoolExpOr
#define B_NOT new kconfig::BoolExpNot
#define B_VAR new kconfig::BoolExpVar
#define B_CONST(v) (kconfig::BoolExpConst::getInstance(v))


namespace kconfig {

    class BoolVisitor;

    enum BoolType {
        NONE, CONST, VAR, NOT, AND, OR
    };

    enum TristateRelation
    {
        rel_yes, rel_mod, rel_pres, rel_helper, rel_meta
    };

    const std::string TristateRelationNames[] = {
        "", "_MODULE", "_PRESENT", "", "_META"
    };

    class BoolExp {
        protected:
            std::string name;
        public:
            bool gcMarked;
            BoolExp *left;
            BoolExp *right;
            int CNFVar;
            BoolExp(): gcMarked(false), left(NULL), right(NULL), CNFVar(0) {}
            virtual std::string str(void);
            virtual BoolExp *simplify(bool);
            virtual int getEvaluationPriority(void) const {return -1;}
            virtual bool equals(const BoolExp *other) const;
            virtual void accept(BoolVisitor *visitor);
            virtual std::string getName(void) const {return this->name;}
            virtual bool isPersistent(void) const {return false;};
            virtual ~BoolExp();
            static BoolExp *parseString(std::string);
    };

    std::ostream& operator<< (std::ostream &s, BoolExp &exp);

    class BoolExpAnd : public BoolExp {
        public:
            BoolExpAnd(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 50;}
    };

    class BoolExpOr : public BoolExp {
        public:
            BoolExpOr(BoolExp *el, BoolExp *er): BoolExp() {
                right = er;
                left = el;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 30;}
    };

    class BoolExpAny : public BoolExp {
        public:
            BoolExpAny(std::string name, BoolExp *el, BoolExp *er):BoolExp() {
                right = er;
                left = el;
                this->name = name;
            }

            int getEvaluationPriority(void) const {return 60;}
            void accept(BoolVisitor *visitor);
            bool equals(const BoolExp *other) const;
    };

    class BoolExpImpl : public BoolExp {
        public:
            BoolExpImpl(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 20;}
    };

    class BoolExpEq : public BoolExp {
        public:
            BoolExpEq(BoolExp *el, BoolExp *er) :BoolExp() {
                right = er;
                left = el;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 10;}
    };

    class BoolExpCall : public BoolExp {
        public:
            std::list<BoolExp *> *param;
            BoolExpCall(std::string name, std::list<BoolExp *> *param) :BoolExp() {
                this->name = name;
                this->param = param;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 90;}
            bool equals(const BoolExp *other) const;
    };

    class BoolExpNot : public BoolExp {
        public:
            BoolExpNot(BoolExp *e) :BoolExp() {
                right = e;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 70;}
    };

    class BoolExpConst : public BoolExp {
        public:
            bool value;
        private:
            BoolExpConst(bool val):BoolExp() {
                value = val ;
            }
        public:
            static BoolExpConst *getInstance(bool val);
            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 90;}
            bool equals(const BoolExp *other) const;
            bool isPersistent(void) const {return true;};
    };

    class BoolExpVar : public BoolExp {
        public:
        TristateRelation rel;

        public:
            BoolExpVar(std::string name, bool addPrefix=true):BoolExp() {
                this->rel = rel_helper;
                this->name = addPrefix ? "CONFIG_" + name: name;
            }

            void accept(BoolVisitor *visitor);
            int getEvaluationPriority(void) const {return 90;}

            bool equals(const BoolExp *other) const;
    };

    BoolExp &operator &&(BoolExp &l, BoolExp &r);
    BoolExp &operator ||(BoolExp &l, BoolExp &r);
    BoolExp &operator !(BoolExp &l);
}

#endif
