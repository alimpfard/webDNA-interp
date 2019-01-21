#include "webdnaAST.hpp"
extern "C" double expr_result;
extern "C" void yyexprparse();
extern "C" void yyexpr_scan_string(const char*);

#define LIBRARY_FUNCTION(name) Value name(int count, std::vector<Value*> args, BlockExprNode inside, EvalContext& ctx)

LIBRARY_FUNCTION(date);
LIBRARY_FUNCTION(showif);
LIBRARY_FUNCTION(mexpr);
LIBRARY_FUNCTION(ifexpr);
LIBRARY_FUNCTION(comment);

// LIBRARY_FUNCTION(fnexpr);
