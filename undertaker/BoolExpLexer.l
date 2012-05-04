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



%{
#include <string>
#include <list>

#include "BoolExpLexer.h"

typedef kconfig::BoolExpParser::token token;
typedef kconfig::BoolExpParser::token_type token_type;

#define yyterminate() return token::END

%}


%option c++
%option prefix="Kconfig"

%option batch

%option debug

%option yywrap nounput

%option stack

%{
#define YY_USER_ACTION  yylloc->columns(yyleng);
%}

%%

 /* init*/
%{
    yylloc->step();
%}


"0"                      return token::TFALSE;
"1"                      return token::TTRUE;
[0-9]+                   return token::TTRUE; /* FIXME int type*/
\'.\'                    return token::TTRUE; /* FIXME char type; fixme chars/escapeseq */

[A-Za-z_][A-Za-z0-9_\.]* { yylval->stringVal = new std::string(yytext, yyleng);
                          return token::STRING;
                         }

"||"                     return token::TOR;
"&&"                     return token::TAND;
"<->"                    return token::TEQ;
"->"                     return token::TIMPL;

"?"                      return token::TCOP;
":"                      return token::TCOP;
"&"                      return token::TCOP;
"|"                      return token::TCOP;
">="                     return token::TCOP;
"<="                     return token::TCOP;
"=="                     return token::TCOP;
"!="                     return token::TCOP;
">>"                     return token::TCOP;
"<<"                     return token::TCOP;
"<<<"                    return token::TCOP;
">"                      return token::TCOP;
"<"                      return token::TCOP;
"*"                      return token::TCOP;
"/"                      return token::TCOP;
"+"                      return token::TCOP;
"-"                      return token::TCOP;
"%"                      return token::TCOP;


[ \t\r]+                 {yylloc->step();}
\n                       {yylloc->lines(yyleng); yylloc->step();}

.                        {return static_cast<token_type>(*yytext);}

%%

namespace kconfig {

BoolExpLexer::BoolExpLexer(std::istream* in,
         std::ostream* out)
    : KconfigFlexLexer(in, out)
{
}

BoolExpLexer::~BoolExpLexer(){}

void BoolExpLexer::set_debug(bool b)
{
    yy_flex_debug = b;
}

}

/* dummydecl */
#ifdef yylex
#undef yylex
#endif

int KconfigFlexLexer::yylex()
{
    std::cerr << "function yylex()" << std::endl;
    return 0;
}

int KconfigFlexLexer::yywrap()
{
    return 1;
}
