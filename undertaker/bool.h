// -*- mode: c++ -*-
/*
 *   boolean framework for undertaker and satyr
 *
 * Copyright (C) 2012 Ralf Hackner <rh@ralf-hackner.de>
 * Copyright (C) 2013 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
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

#ifndef KCONFIG_BOOL_H
#define KCONFIG_BOOL_H

#include "SymbolTools.h"

#include <string>
#include <list>
#include <ostream>

#define B_AND new kconfig::BoolExpAnd
#define B_OR new kconfig::BoolExpOr
#define B_NOT new kconfig::BoolExpNot
#define B_IMPL new kconfig::BoolExpImpl
#define B_VAR new kconfig::BoolExpVar
#define B_CONST(v) (kconfig::BoolExpConst::getInstance(v))


namespace kconfig {
    class BoolVisitor;

    enum TristateRelation { rel_yes, rel_mod, rel_pres, rel_helper, rel_meta };

    const std::string TristateRelationNames[] = {"", "_MODULE", "_PRESENT", "", "_META"};

/************************************************************************/
/* BoolExp                                                              */
/************************************************************************/

    class BoolExp {
    protected:
        std::string name;
    public:
        bool gcMarked = false;
        BoolExp *left = nullptr;
        BoolExp *right = nullptr;
        int CNFVar = 0;

        BoolExp() = default;
        virtual ~BoolExp();
        std::string str(void);
        const std::string &getName(void) const { return this->name; }
        //! Apply obvious simplifications (if possible)
        BoolExp *simplify();

        virtual int getEvaluationPriority(void) const { return -1; }
        virtual bool equals(const BoolExp *other) const;
        virtual void accept(BoolVisitor *visitor);

        static BoolExp *parseString(std::string);
    };

/************************************************************************/
/* BoolExpAnd                                                           */
/************************************************************************/

    class BoolExpAnd : public BoolExp {
    public:
        BoolExpAnd(BoolExp *el, BoolExp *er) {
            right = er;
            left = el;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 50; }
    };

/************************************************************************/
/* BoolExpOr                                                            */
/************************************************************************/

    class BoolExpOr : public BoolExp {
    public:
        BoolExpOr(BoolExp *el, BoolExp *er) {
            right = er;
            left = el;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 30; }
    };

/************************************************************************/
/* BoolExpAny                                                           */
/************************************************************************/

    class BoolExpAny : public BoolExp {
    public:
        BoolExpAny(std::string name, BoolExp *el, BoolExp *er) {
            right = er;
            left = el;
            this->name = name;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 60; }
        virtual bool equals(const BoolExp *other) const final override;
    };

/************************************************************************/
/* BoolExpImpl                                                          */
/************************************************************************/

    class BoolExpImpl : public BoolExp {
    public:
        BoolExpImpl(BoolExp *el, BoolExp *er) {
            right = er;
            left = el;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 20; }
    };

/************************************************************************/
/* BoolExpEq                                                            */
/************************************************************************/

    class BoolExpEq : public BoolExp {
    public:
        BoolExpEq(BoolExp *el, BoolExp *er) {
            right = er;
            left = el;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 10; }
    };

/************************************************************************/
/* BoolExpCall                                                          */
/************************************************************************/

    class BoolExpCall : public BoolExp {
    public:
        std::list<BoolExp *> *param;
        BoolExpCall(std::string name, std::list<BoolExp *> *param) {
            this->name = name;
            this->param = param;
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 90; }
        virtual bool equals(const BoolExp *other) const final override;
    };

/************************************************************************/
/* BoolExpNot                                                           */
/************************************************************************/

    class BoolExpNot : public BoolExp {
    public:
        BoolExpNot(BoolExp *e) { right = e; }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 70; }
    };

/************************************************************************/
/* BoolExpConst                                                         */
/************************************************************************/

    class BoolExpConst : public BoolExp {
        BoolExpConst(bool val) { value = val; }

    public:
        bool value;

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 90; }
        virtual bool equals(const BoolExp *other) const final override;

        static BoolExpConst *getInstance(bool val);
    };

/************************************************************************/
/* BoolExpVar                                                           */
/************************************************************************/

    class BoolExpVar : public BoolExp {
    public:
        TristateRelation rel;
        struct symbol *sym;

        BoolExpVar(std::string name, bool addPrefix = true) {
            this->rel = rel_helper;
            this->name = addPrefix ? "CONFIG_" + name : name;
        }

        BoolExpVar(struct symbol *sym, TristateRelation rel) {
            nameSymbol(sym);
            std::string name(sym->name ? sym->name : "[unnamed_menu]");
            this->rel = rel;
            this->sym = sym;
            this->name = "CONFIG_" + name + TristateRelationNames[rel];
        }

        virtual void accept(BoolVisitor *visitor) final override;
        virtual int getEvaluationPriority(void) const final override { return 90; }
        virtual bool equals(const BoolExp *other) const final override;
    };

/************************************************************************/
/* Operators                                                            */
/************************************************************************/

    std::ostream &operator<<(std::ostream &s, BoolExp &exp);

    BoolExp &operator&&(BoolExp &l, BoolExp &r);
    BoolExp &operator||(BoolExp &l, BoolExp &r);
    BoolExp &operator!(BoolExp & l);
}
#endif
