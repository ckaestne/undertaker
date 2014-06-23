// -*- mode: c++ -*-
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

#ifndef KCONFIG_BOOLEXPSTRINGBUILDER_H
#define KCONFIG_BOOLEXPSTRINGBUILDER_H

#include "bool.h"
#include "BoolVisitor.h"

namespace kconfig {
    class BoolExpStringBuilder : public BoolVisitor {
    public:
        std::string str(void) const{
            return *(static_cast<std::string *>(this->result));
        }

        ~BoolExpStringBuilder() {
            for (auto &entry : visited)  // pair<BoolExp *, void *>
                if (entry.second)
                    delete static_cast<std::string *>(entry.second);
        }

    protected:
        virtual void visit(BoolExp *)       final override {
            this->result = new std::string("__something_went_horribly_wrong__");
        }

        virtual void visit(BoolExpAnd *e)   final override {
            makestr(" && ", e);
        }

        virtual void visit(BoolExpOr *e)    final override {
            makestr(" || ", e);
        }

        virtual void visit(BoolExpNot *e)   final override {
            makestr("!", e);
        }

        virtual void visit(BoolExpConst *e) final override {
            const char *v = e->value ? "1" : "0";
            this->result = new std::string(v);
        }

        virtual void visit(BoolExpVar *e)   final override {
            this->result = new std::string(e->getName());
        }

        virtual void visit(BoolExpImpl *e)  final override {
            makestr(" -> ", e);
        }

        virtual void visit(BoolExpEq *e)    final override {
            makestr(" <-> ", e);
        }

        virtual void visit(BoolExpCall *e)  final override {
            bool first = true;
            std::string paramstring("");
            for (auto &ptr : *e->param) {  // BoolExp *
                paramstring += first ? "" : ", ";
                paramstring += ptr->str();
                first = false;
            }
            this->result = new std::string{ e->getName() + " (" + paramstring + ")" };
        }

        virtual void visit(BoolExpAny *e)   final override {
            makestr({ " " + e->getName() + " " }, e);
        }

    private:
        static const char *lbrace (BoolExp *parent, BoolExp *child) {
            return (child->getEvaluationPriority() >= parent->getEvaluationPriority()) ? "" : "(";
        }

        static const char *rbrace (BoolExp *parent, BoolExp *child) {
            return (child->getEvaluationPriority() >= parent->getEvaluationPriority()) ? "" : ")";
        }

        void makestr(const std::string &op, BoolExp *e) {
            std::string *l = static_cast<std::string *>(left);
            std::string *r = static_cast<std::string *>(right);
            std::string *s = new std::string();
            if (l) {
                *s += lbrace(e, e->left) + *l + rbrace(e, e->left);
            }
            *s += op;
            if (r) {
                *s += lbrace(e, e->right) + *r + rbrace(e, e->right);
            }
            this->result = s;
        }
    };
}

#endif
