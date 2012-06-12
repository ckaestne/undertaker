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



#ifndef KCONFIG_CNFBUILDER_H
#define KCONFIG_CNFBUILDER_H

#include "bool.h"
#include "CNF.h"

#include "KconfigWhitelist.h"

#include <string>
#include <sstream>
#include <map>

namespace kconfig
{
    class CNFBuilder : public BoolVisitor
    {
        public:
            enum ConstantPolicy {BOUND = 0, FREE};
        private:
            int boolvar;

            KconfigWhitelist *wl;

            //FIXME:
            //dirty workaround! should be done in Parser!
            //will conflict as soon const node unification is working
            enum ConstantPolicy constPolicy;

        public:
            CNF *cnf;

            CNFBuilder(bool useKconfigWhitelist=false, enum ConstantPolicy constPolicy=BOUND);

            //! Add clauses from the parsed boolean expression e
            /**
             * \param[in,out] e the parsed expression
             *
             * Note that this triggers the traversal on the tree e,
             * which (as every visitor), and therefrore potentially
             * modifies the CNF variable assignment.
             */
            void pushClause(BoolExp *e);

            //! Add new variable to the CNF and returns associated var number
            /**
             * @param[in] the name of the variable
             * @returns associated var number
             *
             * Adds a new variable to the cnf and returns the associated
             * cnf var number. If a variable with the given name already
             * exists, the var number of the existing var is returned.
             */
            int addVar(std::string s);

        protected:
            virtual void visit(BoolExp *e);
            virtual void visit(BoolExpAnd *e);
            virtual void visit(BoolExpOr *e);
            virtual void visit(BoolExpNot *e);
            virtual void visit(BoolExpConst *e);
            virtual void visit(BoolExpVar *e);
            virtual void visit(BoolExpImpl *e);
            virtual void visit(BoolExpEq *e);
            virtual void visit(BoolExpCall *e);
            virtual void visit(BoolExpAny *e);
    };
}
#endif
