%{
  #include "webdnaAST.hpp"
  #include "webdna_syntax.hpp"
  #include <stdarg.h>
  extern "C" int yylex(void);
  extern "C" int yy_top_state(void);

  int lexLineNumber = 0;
  int modeRaw = 0;
  #define TOKEN(tok) (yylval.token = tok)

  /**
   * Debug mode flag
   * Set to 1 to print lexer states and debug stuff
   */
  int debug_mode = 0;
  void dprintf(char* fmt, ...) {
    if (!debug_mode) return;
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
  }
  #define PDEBUG(name) dprintf("[" #name "] %.*s (%d) state=%s\n", yyleng, yytext, modeRaw, state_table[YYSTATE])
%}

WC      [A-Za-z']
NW      [^A-Za-z']

%start  INW NIW LTR

%%
  /* add a simple lookup table for states */
  static char* state_table[] = {
    [INITIAL] = "INITIAL",
    [INW]     = "INW",
    [NIW]     = "NIW",
    [LTR]     = "LTR",
  };

  /* Detect beginning and end of word */
<INITIAL,INW,NIW>{WC}                                                           { BEGIN INW; REJECT; }
<INITIAL,INW,NIW>{NW}                                                           { BEGIN NIW; REJECT; }

  /* Brackets */
"["                                                                             {
                                                                                  PDEBUG(ob);
                                                                                  BEGIN NIW;
                                                                                  modeRaw++;
                                                                                  return TOKEN(OBRACK);
                                                                                }
  /* Bracket */
"]"                                                                             {
                                                                                  PDEBUG(cb);
                                                                                  BEGIN NIW;
                                                                                  if (!modeRaw) {
                                                                                    REJECT;
                                                                                  } else {
                                                                                    modeRaw--;
                                                                                    return TOKEN(CBRACK);
                                                                                  }
                                                                                }
  /* In place of [raw]...[/raw], since screw that */
(?s:["][^"]*["])                                                                {
                                                                                  PDEBUG(st);
                                                                                  yylval.stringvalue = new std::string(yytext+1, yyleng-2);
                                                                                  return STRING;
                                                                                }
  /* consume newlines, reject if in literal state */
\n                                                                              {
                                                                                  PDEBUG(nl);
                                                                                  lexLineNumber++;
                                                                                  if (YYSTATE == LTR) {
                                                                                    REJECT;
                                                                                  }
                                                                                }
  /* Skip whitespace */
[ \t\r\n]+                                                                      {
                                                                                  PDEBUG(sps);
                                                                                  if (YYSTATE == LTR) {
                                                                                    REJECT;
                                                                                  }
                                                                                }
  /* Detect starting/ending tag names */
<NIW>(\/?)[!~@\-=*&^%$#/a-zA-Z_][!~@\-=*&^%$#/a-zA-Z0-9_]*/{NW}                 {
                                                                                  PDEBUG(id);
                                                                                  if (modeRaw == 0) {
                                                                                    REJECT;
                                                                                  } else {
                                                                                    yylval.stringvalue = new std::string(yytext, yyleng);
                                                                                    return IDENT;
                                                                                  }
                                                                                }
  /* Numbers (not strictly necessary) */
[0-9]+                                                                          {
                                                                                  PDEBUG(nm);
                                                                                  if (modeRaw == 0) {
                                                                                    REJECT;
                                                                                  } else {
                                                                                    yylval.stringvalue = new std::string(yytext, yyleng);
                                                                                    return NUMBER;
                                                                                  }
                                                                                }
  /* Copy any character verbatim if in literal mode */
<LTR>.                                                                          {
                                                                                  PDEBUG(lt);
                                                                                  BEGIN LTR;
                                                                                  yylval.stringvalue = new std::string(yytext, yyleng);
                                                                                  return LITERAL;
                                                                                }
  /* If we reached here, we've seen some character that doesn't start any sequence_tag
      Follow on and start literal mode */
.                                                                               {
                                                                                  PDEBUG(ltx);
                                                                                  BEGIN LTR;
                                                                                  yylval.stringvalue = new std::string(yytext, yyleng);
                                                                                  return LITERAL;
                                                                                }

%%

int yywrap(void){return 1;}
