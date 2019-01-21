#include "webdnaAST.hpp"
#include <cstring>
#include "internal_functions.hpp"
#include "stdarg.h"
#include <numeric>
#include <sstream>

extern std::string to_text(Value);
extern bool to_bool(Value);
extern std::string strip(std::string);

LIBRARY_FUNCTION(date) {
    char buf[1024];
    Value* fmtv = args.size() > 0 ? args[0] : nullptr;

    const char* format = fmtv ? to_text(*fmtv).c_str() : "%c";
    struct tm *tm;
    time_t rawtime;

    time (&rawtime);
    tm = localtime(&rawtime);

    strftime(buf, 1024, format, tm);
    return {{1}, {strdup(buf)}};
}

std::string join_array(std::vector<std::string> arr, std::string del) {
    std::string s;
    s = std::accumulate(arr.begin(), arr.end(), s, [&](decltype(s) s0, decltype(s) s1){return s0 + del + s1;});
    return s;
}
std::string join_array_reverse(std::vector<std::string> arr, std::string del) {
    std::string s;
    s = std::accumulate(arr.rbegin(), arr.rend(), s, [&](decltype(s) s0, decltype(s) s1){return s0 + del + s1;});
    return s;
}

bool boolean_expr(std::vector<Value*> inside, EvalContext& ctx) {
    std::vector<std::string> expr = fmap([](Value* x){ return to_text(*x); }, inside);
    // std::cout << s << '\n';
    yyexpr_scan_string((join_array(expr, "") + "\n").c_str());
    yyexprparse();
    return expr_result == 1;
}
std::string string_expr(std::vector<Value*> inside, EvalContext& ctx) {
    std::vector<std::string> expr = fmap([](Value* x){ return to_text(*x); }, inside);
    std::string s = join_array(expr, "") + "\n";
    // std::cout << s << '\n';
    yyexpr_scan_string(s.c_str());
    yyexprparse();
    std::ostringstream ss;
    ss << expr_result;
    return ss.str();
}

LIBRARY_FUNCTION(showif) {
    if (args.size() < 1)
        return {{1}, {""}};
    if (boolean_expr(args, ctx))
        return {{1}, {strdup(join_array(fmap([&](ExprNode* x) { return to_text(x->evaluate(ctx)); }, inside.code), "\n").c_str())}};
    return {{1}, {""}};
}

LIBRARY_FUNCTION(mexpr) {
    if (args.size() < 1)
        return {{1}, {""}};
    return {{1}, {strdup(string_expr(args, ctx).c_str())}};
}

LIBRARY_FUNCTION(ifexpr) {
    if (boolean_expr(args, ctx)) {
        // see if there's a "then" branch
        for (auto nit = inside.code.begin(); nit != inside.code.end(); ++nit)
        {
            ExprNode* node = *nit;
            if (node->ty() == 1) {
                InterpredText* cand = dynamic_cast<InterpredText*>(node);
                if (cand->name == "then") {
                    return cand->contained.evaluate(ctx);
                }
            }
        }
    } else {
        // see if there's an "else" branch
        for (auto nit = inside.code.begin(); nit != inside.code.end(); ++nit)
        {
            ExprNode* node = *nit;
            if (node->ty() == 1) {
                InterpredText* cand = dynamic_cast<InterpredText*>(node);
                if (cand->name == "else") {
                    return cand->contained.evaluate(ctx);
                }
            }
        }
    }
    return {{1},{""}};
}

LIBRARY_FUNCTION(comment) {
    return {{1},{""}};
}

LIBRARY_FUNCTION(fnexpr) {
    if (args.size() < 1)
        return {{1}, {0}};

    std::string name = to_text(*args[0]);
    std::vector<std::string> fnargs;
    bool params = false;
    BlockExprNode body;
    body.parsed = true;

    for (auto nit = inside.code.begin(); nit != inside.code.end(); ++nit)
    {
        bool skip = false;
        ExprNode* node = *nit;
        if (node->ty() == 1) {
            InterpredText* cand = dynamic_cast<InterpredText*>(node);
            if (cand->name == "params" && !params) {
                skip = true;
                params = true;
                fnargs = fmap([](ExprNode* node){
                        if (node->ty() != 2) return std::string("");
                        return strip(((LiteralText*) node)->value);
                    }, cand->contained.code);
            }
        }
        if (params && !skip) {
            body.code.push_back(node);
        }
    }
    ctx.top()->putSymbol(name,
        TagDescriptor {
            static_cast<int>(fnargs.size()),
            true,
            1,
            {.user_defined_fn = new FunctionNode(name, fnargs, body)}
        }
    );
    return {{1},{""}};
}

LIBRARY_FUNCTION(strconcat) {
    std::vector<std::string> expr = fmap([&](ExprNode* x){ return to_text(x->evaluate(ctx)); }, inside.code);
    return {{1}, {strdup(join_array(expr, "").c_str())}};
}
