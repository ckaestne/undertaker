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

%{

#include <stdio.h>
#include <string>
#include <list>

#include "bool.h"
#include "exceptions/BoolExpParserException.h"

%}

%require "2.3"
%debug
%start Line

%defines

%skeleton "lalr1.cc"

%name-prefix "kconfig"
%define parser_class_name {BoolExpParser}

%locations

%parse-param { class BoolExp **result }
%parse-param { class BoolExpLexer *lexer }

%error-verbose

%union {
    std::string*      stringVal;
    class BoolExp*    boolNode;
    class std::list<BoolExp *>* paramList;
}

%token               END      0   "eof"
%token <stringVal>   STRING       "string"
%token               TOR          "or"
%token               TAND         "and"
%token               TEQ          "biimplication"
%token               TIMPL        "implication"
%token               TTRUE        "true"
%token               TFALSE       "false"
%token <stringVal>   TCOP         "C-Operator"

%type <boolNode>     Const
%type <boolNode>     Atom Literal AndExpr OrExpr ImplExpr EqExpr Expr CExpr
%type <paramList>    ParamList

%destructor { delete $$; } STRING
%destructor { delete $$; } Const
%destructor { delete $$; } Atom Literal AndExpr OrExpr ImplExpr EqExpr CExpr Expr ParamList

%{
#include "BoolExpLexer.h"

#undef yylex
#define yylex this->lexer->lex
%}

%%

Const :    TTRUE                {$$ = B_CONST(true);}
         | TFALSE               {$$ = B_CONST(false);}
         ;

Atom :     Const               {$$ = $1;}
         | STRING '(' ParamList ')'  {$$ = new BoolExpCall(*$1, $3); delete $1;}
         | STRING              {$$ = new BoolExpVar(*$1, false); delete $1;}
         | '(' Expr ')'        {$$ = $2;}
         ;

ParamList : /*void*/            { $$ = new std::list<BoolExp *>();}
          | Expr                { $$ = new std::list<BoolExp *>(); $$->push_back($1);}
          | ParamList ',' Expr  { $$ = $1; $1->push_back($3);}
          ;

Literal : Atom                 {$$ = $1;}
         | '!' Literal         {$$ = new BoolExpNot($2);}
         ;

CExpr : Literal                {$$ = $1;}
      | CExpr TCOP Literal     {$$ = new BoolExpAny(*$2, $1, $3); delete $2;}
      ;

AndExpr : CExpr                {$$ = $1;}
        | AndExpr TAND CExpr   {$$ = new BoolExpAnd($1, $3);}
        ;

OrExpr : AndExpr               {$$ = $1;}
       | OrExpr TOR AndExpr    { $$ = new BoolExpOr($1, $3);}
       ;

ImplExpr :OrExpr               {$$ = $1;}
       | ImplExpr TIMPL OrExpr { $$ = new BoolExpImpl($1, $3);}
       ;

EqExpr : ImplExpr              {$$ = $1;}
       | EqExpr TEQ ImplExpr   { $$ = new BoolExpEq($1, $3);}
       ;

Expr    : EqExpr               {$$ = $1;}
        ;

Line    : Expr END             { *(this->result) = $1;}
        ;
%%
void kconfig::BoolExpParser::error(const BoolExpParser::location_type&, const std::string& msg){
    throw new BoolExpParserException(msg);
}
