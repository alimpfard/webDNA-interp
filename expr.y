%{
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

double expr_result;

struct svalue {
  char* val;
  int length;
};

typedef struct {
  int ty : 1;
  union {
    double nval;
    struct svalue sval;
  };
} id_or_number;

#define NODEFINEIDNUM

static char* expr_tostr(id_or_number val) {
  char buf[1024];
  if (val.ty == 0)
    sprintf(buf, "%f", val.nval);
  else
    sprintf(buf, "%.*s", val.sval.length, val.sval.val);
  return strdup(buf);
}
static double expr_tonum(id_or_number val) {
  if (val.ty == 0)
    return val.nval;
  else {
    char buf[100];
    sprintf(buf, "%.*s", val.sval.length, val.sval.val);
    return atof(buf);
  }
}
static id_or_number num_to_expr(double num) {
  id_or_number val = {
    .ty = 0,
    .nval = num
  };
  return val;
}
%}

%define api.prefix {yyexpr}

%code requires {
  #ifndef NODEFINEIDNUM
  struct svalue {
    char* val;
    int length;
  };

  typedef struct {
    int ty : 1;
    union {
      double nval;
      struct svalue sval;
    };
  } id_or_number;
  #endif
  #define YYEXPRSTYPE id_or_number
}

%token NUMBER STRING
%token PLUS MINUS TIMES DIVIDE POWER EQUALS NEQUALS
%token LEFT RIGHT
%token END

%left EQUALS NEQUALS
%left PLUS MINUS
%left TIMES DIVIDE
%left NEG
%right POWER

%start Line

%%
Line: Expression END                                                            { expr_result = expr_tonum($1); }
    ;

Expression: NUMBER                                                              { $$ = $1; }
          | STRING                                                              { $$ = $1; }
          | Expression PLUS Expression                                          { $$ = num_to_expr(expr_tonum($1) + expr_tonum($3)); }
          | Expression MINUS Expression                                         { $$ = num_to_expr(expr_tonum($1) - expr_tonum($3)); }
          | Expression TIMES Expression                                         { $$ = num_to_expr(expr_tonum($1) * expr_tonum($3)); }
          | Expression DIVIDE Expression                                        { $$ = num_to_expr(expr_tonum($1) / expr_tonum($3)); }
          | MINUS Expression %prec NEG                                          { $$ = num_to_expr(-expr_tonum($2)); }
          | Expression POWER Expression                                         { $$ = num_to_expr(pow(expr_tonum($1),expr_tonum($3))); }
          | LEFT Expression RIGHT                                               { $$ = $2; }
          | Expression EQUALS Expression                                        { $$ = num_to_expr(strcmp(expr_tostr($1), expr_tostr($3)) == 0); }
          | Expression NEQUALS Expression                                       { $$ = num_to_expr(strcmp(expr_tostr($1), expr_tostr($3)) != 0); }
          ;

%%

int yyexprerror(char *s) {
  printf("%s\n", s);
}
