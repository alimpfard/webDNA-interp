%{
  #include "webdnaAST.hpp"
  extern int modeRaw;
  extern "C" void yyerror(char *) {};
  extern "C" int yylex(void);
  BlockExprNode *root;
%}

%union {
  int token;
  std::string* stringvalue;
  std::vector<ExprNode*>* list;
  ExprNode* text;
  BlockExprNode* block;
}

%token <stringvalue> IDENT NUMBER STRING LITERAL              /* String types */
%token <token> CBRACK OBRACK                                  /* Tag modifier */

%type <list> maybe_arguments
%type <text> interpret literal text literalv
%type <block> global_block

%%

program : global_block { root = $1; }
        ;

global_block : text                                                             { $$ = new BlockExprNode(); $$->code.insert($$->code.begin(), $1); }
             | text global_block                                                { $$ = $2; $$->code.insert($$->code.begin(), $1); }
             ;

text : interpret                                                                { $$ = $1; }
     | literalv                                                                 { $$ = $1; }
     ;

interpret : OBRACK IDENT maybe_arguments CBRACK                                 { $$ = new InterpredText(*$2, *$3); delete $2; delete $3; }
          ;

literal   : LITERAL                                                             { $$ = new LiteralText(*$1); delete $1; }
          | NUMBER                                                              { $$ = new LiteralText(*$1); delete $1; }
          | STRING                                                              { $$ = new LiteralText(*$1); delete $1; }
          ;

/* Concatenate sequential LITERAL texts together, since we detect them one
 * character at a time
 */
literalv  : literal                                                             { $$ = $1; }
          | literal literalv                                                    { $$ = $1; ((LiteralText*)$$)->value += ((LiteralText*)$2)->value; }

maybe_arguments : literalv  maybe_arguments                                     { $$ = $2; $$->insert($$->begin(), $1); }
                | interpret maybe_arguments                                     { $$ = $2; $$->insert($$->begin(), $1); }
                | IDENT     maybe_arguments                                     { $$ = $2; $$->insert($$->begin(), new LiteralText(*$1)); delete $1; }
                |                                                               { $$ = new std::vector<ExprNode*>(); }
                ;
