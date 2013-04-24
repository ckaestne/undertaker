// -*- mode: c++ -*-
/*
 *   boolean framework for undertaker and satyr
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


#ifndef KCONFIG_BOOLEXPLEXER_H
#define KCONFIG_BOOLEXPLEXER_H

#ifndef YY_DECL

#define YY_DECL \
    kconfig::BoolExpParser::token_type \
    kconfig::BoolExpLexer::lex( \
    kconfig::BoolExpParser::semantic_type* yylval, \
    kconfig::BoolExpParser::location_type* yylloc \
    )
#endif

#ifndef __FLEX_LEXER_H
#define yyFlexLexer KconfigFlexLexer
#include "FlexLexer.h"
#undef yyFlexLexer
#endif

#include <list>

#include "BoolExpParser.h"

namespace kconfig {
    class BoolExpLexer : public KconfigFlexLexer {
    public:
        BoolExpLexer(std::istream* arg_yyin = 0, std::ostream* arg_yyout = 0);

        virtual ~BoolExpLexer();

        virtual BoolExpParser::token_type lex(
                BoolExpParser::semantic_type* yylval,
                BoolExpParser::location_type* yylloc);

        void set_debug(bool b);
    };
}
#endif
