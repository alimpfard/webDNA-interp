#include "webdnaAST.hpp"
#include <cstring>
#include <iostream>
#include "internal_functions.hpp"

extern "C" double expr_result;


extern BlockExprNode* root;
extern "C" FILE* yyin;
extern int lexLineNumber;

std::string to_text(Value value) {
    switch (value.info.tag) {
    case 1:
        if (value.text)
            return std::string (strdup(value.text));
        else return "";
    }
    return "[Value]";
}

bool to_bool (Value value) {
    return value.number > 0;
}

Value BlockExprNode::evaluate(EvalContext& ctx) {
    std::string response;
    for (auto iter = code.begin(); iter != code.end(); ++iter)
    {
        ExprNode* node = *iter;
        if (!parsed) {
            switch (node->ty()) {
            case 2:
                break;
            case 1: {
                InterpredText* ipt = dynamic_cast<InterpredText*>(node);
                auto end = code.end();
                ipt->parseForward(iter, end, ctx);
                break;
            }
            case 0:
            default:
                break;
            }
        }
        response.append(to_text(node->evaluate(ctx)));
    }
    parsed = true;
    return {{1}, {strdup(response.c_str())}};
}

bool isSingleTag(InterpredText *tag, EvalContext& ctx) {
    return ctx.tags[tag->name].needs_end;
}

void InterpredText::parseForward(InterpredText::InsideItType& iter, InterpredText::InsideItType& end, EvalContext& ctx) {
    if (isSingleTag(this, ctx))
        return;
    bool okay = false;
    while (++iter != end) {
        ExprNode* node = *iter;
        switch (node->ty()) {
        case 1:
            if (((InterpredText*)node)->name == "/" + name) { okay = true; goto NOMORE; }
            else
                (((InterpredText*)node)->parseForward(iter, end, ctx));
        default:
            contained.code.push_back(node);
        }
    }
    iter--; // compensate for for loop above
    NOMORE:;
    contained.parsed = true;
    if (!okay)
        abort();
}
int level = 0;
void InterpredText::parseForward(InterpredText::ReverseInsideItType& iter, InterpredText::ReverseInsideItType& end, EvalContext& ctx) {
    if (isSingleTag(this, ctx))
        return;
    bool okay = false;
    while (++iter != end) {
        ExprNode* node = *iter;
        auto inode = (InterpredText*)node;
        switch (node->ty()) {
        case 1:
            if (inode->name == "/" + name) {
                okay = true;
                goto NOMORE;
            }
            else {
                (inode->parseForward(iter, end, ctx));
            }
        default:
            contained.code.push_back(node);
        }
    }
    iter--; // compensate for for loop above
    NOMORE:;
    contained.parsed = true;
    if (!okay)
        abort();
}

Value* make_value_ptr(Value value) {
    Value *v = new Value();
    *v = value;
    return v;
}

Value InterpredText::evaluate(EvalContext& ctx) {
    auto desc = ctx.tags[name];
    if (desc.native_fn)
        return desc.native_fn(desc.argument_count, fmap([&](ExprNode* expr){ return make_value_ptr(expr->evaluate(ctx)); }, args), contained, ctx);
    return {};
}

Value LiteralText::evaluate(EvalContext& ctx) {
    return {{1, 0}, {strdup(value.c_str())} };
}

int main() {
    yyin = fopen("in.dna", "r");
	if (!yyin) {
        perror("File opening failed");
        return EXIT_FAILURE;
    }
    if (yyparse()) {
        std::cout << "Syntax Error at line " << lexLineNumber << '\n';
        return 1;
    }
    EvalContext ctx {
        {
            {"date",     {0, true,  date}},
            {"showif",   {1, false, showif}},
            {"math",     {1, true,  mexpr}},
            {"if",       {1, false, ifexpr}},
            {"!",        {0, false, comment}},
            // {"function", {1, !false, fnexpr}},
        },
        {}
    };
    std::cout << to_text(root->evaluate(ctx)) << "\n";
}
