%{
#include "expr.tab.h"
%}

%option prefix="yyexpr"
%option noyywrap

white [ \t]+
digit [0-9]
integer {digit}+
exponent [eE][+-]?{integer}
real {integer}("."{integer})?{exponent}?
ident [a-zA-Z_][a-zA-Z0-9_]*

%%

{white} { }
{real} {
  yyexprlval.ty = 0;
  yyexprlval.nval = atof(yytext);
  return NUMBER;
}
{ident} {
  yyexprlval.ty = 1;
  yyexprlval.sval = (struct svalue){.val = strdup(yytext), .length = yyleng};
  return STRING;
}
(?s:['][^']*[']) {
  yyexprlval.ty = 1;
  yyexprlval.sval = (struct svalue){.val = strdup(yytext+1), .length = yyleng-2};
  return STRING;
}

"+"  return PLUS;
"-"  return MINUS;
"*"  return TIMES;
"/"  return DIVIDE;
"^"  return POWER;
"("  return LEFT;
")"  return RIGHT;
"="  return EQUALS;
"!"  return NEQUALS;
"\n" return END;
