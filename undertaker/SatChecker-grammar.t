/* -*- mode: c++ -*-
 *
 *   undertaker - analyze preprocessor blocks in code
 *
 * Copyright (C) 2009-2012 Reinhard Tartler <tartler@informatik.uni-erlangen.de>
 * Copyright (C) 2010-2012 Christian Dietrich <christian.dietrich@informatik.uni-erlangen.de>
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


using namespace BOOST_SPIRIT_CLASSIC_NS;

///////////////////////////////////////////////////////////////////////////////
//
//  boolean expression parser
//
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <set>

struct bool_grammar : public grammar<bool_grammar>
{
    enum {symbolID = 2, not_symbolID, andID, orID, impliesID, iffID,
          comparatorID,
    };

    typedef std::set<std::string> SymbolSet;


    mutable SymbolSet symbols;

    SymbolSet &get_symbols() const {
        return symbols;
    }

    template <typename ScannerT>
    struct definition
    {

        struct add_symbol {
            add_symbol(SymbolSet &s) : _s(s) {};

            SymbolSet &_s;
            void operator() (const char *first, const char *last) const {
                std::string s(first, last);
                _s.insert(s);
            }
        };

        add_symbol collect_symbols;

        rule<ScannerT, parser_context<>, parser_tag<symbolID> > symbol;
        rule<ScannerT, parser_context<>, parser_tag<not_symbolID> > not_symbol;

        rule<ScannerT, parser_context<>, parser_tag<andID> > and_term;
        rule<ScannerT, parser_context<>, parser_tag<orID> > or_term;
        rule<ScannerT, parser_context<>, parser_tag<impliesID> > implies_term;
        rule<ScannerT, parser_context<>, parser_tag<iffID> > iff_term;
        rule<ScannerT, parser_context<>, parser_tag<comparatorID> > comparator_term;
        rule<ScannerT> start_rule, group, term, expression;

        definition(bool_grammar const& self) : collect_symbols(self.get_symbols())  {
            /*
               Operators (from weak to strong): <->, ->, |, &, !(),
             */
            (void) self;
            symbol       = (lexeme_d[ leaf_node_d[ +(alnum_p | ch_p('_') | ch_p('.') | ch_p('\'') | ch_p('"')) ]]
                >> *(no_node_d [ ch_p('(') >> *(~ch_p(')')) >> ch_p(')')]))[collect_symbols];
            group        = no_node_d[ ch_p('(') ]
                >> start_rule
                >> no_node_d[ ch_p(')') ];
            not_symbol   = no_node_d[ch_p('!')] >> term;

            term = group | not_symbol | symbol;

#define __C_OPERATORS(p) (p(|) | p(^) | p(&) | p(==) | p(!=) | p(<<) | p(>>) \
                          | p(>=) | p(>) | p(<=) | p(<)                 \
                          | p(+)  | p(-) | p(*)  | p(/) | p(%) \
                          | p(?) | p(:))
#define __my_str_p(s) str_p( #s )
            comparator_term = term >> *( __C_OPERATORS(__my_str_p) >> term);

            /* nodes that aren't leaf nodes */
            and_term     = comparator_term >> *(no_node_d[str_p("&&")] >> comparator_term);
            or_term      = and_term        >> *(no_node_d[str_p("||")] >> and_term);

            implies_term = or_term      >> *(no_node_d[str_p("->")] >> or_term);
            iff_term     = implies_term
                >> *(no_node_d[ str_p("<->") ] >> implies_term);

            start_rule = iff_term
                >> no_node_d[ *(space_p | ch_p("\n") | ch_p("\r"))];
        };

        rule<ScannerT> const& start() const { return start_rule; }
    };
};
