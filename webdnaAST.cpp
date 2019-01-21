#include "webdnaAST.hpp"
#include <cstring>
#include <iostream>
#include "internal_functions.hpp"
#include "zip.hpp"

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
    return "[?]";
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
    return ctx.findSymbol(tag->name).is_single_tag;
}

std::string strip(std::string ins) {
    char *in = const_cast<char*>(ins.c_str()), *end = in;
    end += ins.size();

    char c;
    while((c = *in++)) {
        if (!(
            c == ' ' || c == '\t' || c == '\r' || c == '\n'
        )) break;
    }
    in--;
    char* start = in;
    end--;
    while((start < --end)) {
        if (!(
            c == ' ' || c == '\t' || c == '\r' || c == '\n'
        )) break;
    }
    return std::string(start, end-start);
}

void InterpredText::parseForward(InterpredText::InsideItType& iter, InterpredText::InsideItType& end, EvalContext& ctx) {
    if (isSingleTag(this, ctx))
        return;
    bool okay = false;
    if (name == "function") {
        std::string iname = to_text(args[0]->evaluate(ctx));
        ctx.top()->putSymbol(iname, TagDescriptor { 0, true, 2, {nullptr} });
        ctx.pushBlock();
    }
    while (++iter != end) {
        ExprNode* node = *iter;
        switch (node->ty()) {
        case 2:
            if (name == "params") {
                std::string iname = strip(((LiteralText*)node)->value);
                ctx.top()->putSymbol(iname, TagDescriptor { 0, true, 2, {nullptr}});
            }
            goto skip1;
        case 1:
            if (((InterpredText*)node)->name == "/" + name) { okay = true; goto NOMORE; }
            else if (name == "!") goto skip2;
            else
                ((InterpredText*)node)->parseForward(iter, end, ctx);
        default:
            skip1:
            contained.code.push_back(node);
            skip2:;
        }
    }
    iter--; // compensate for for loop above
    NOMORE:;
    contained.parsed = true;
    if (name == "function")
        ctx.popBlock();
    if (!okay)
        abort();
}
int level = 0;
void InterpredText::parseForward(InterpredText::ReverseInsideItType& iter, InterpredText::ReverseInsideItType& end, EvalContext& ctx) {
    if (isSingleTag(this, ctx))
        return;
    bool okay = false;
    if (name == "function") {
        std::string iname = to_text(args[0]->evaluate(ctx));
        ctx.top()->putSymbol(iname, TagDescriptor { 0, true, 2, {nullptr} });
        ctx.pushBlock();
    }
    while (++iter != end) {
        ExprNode* node = *iter;
        auto inode = (InterpredText*)node;
        switch (node->ty()) {
        case 2:
            if (name == "params") {
                std::string iname = strip(((LiteralText*)node)->value);
                ctx.top()->putSymbol(iname, TagDescriptor { 0, true, 2, {nullptr}});
            }
            goto skip1;
        case 1:
            if (inode->name == "/" + name) {
                okay = true;
                goto NOMORE;
            }
            else if (name == "!") goto skip2;
            else {
                inode->parseForward(iter, end, ctx);
            }
        default:
            skip1:
            contained.code.push_back(node);
            skip2:;
        }
    }
    iter--; // compensate for for loop above
    NOMORE:;
    contained.parsed = true;
    if (name == "function")
        ctx.popBlock();
    if (!okay)
        abort();
}

Value* make_value_ptr(Value value) {
    Value *v = new Value();
    *v = value;
    return v;
}


Value FunctionNode::evaluate(EvalContext& ctx) {
    auto frame = ctx.pushBlock();
    for (auto pa : zip(params, args)) {
        auto param = std::get<0>(pa);
        auto arg   = std::get<1>(pa);

        frame->putSymbol(param, TagDescriptor {
            0, true, 2, {.value = make_value_ptr(arg->evaluate(ctx))}
        });
    }
    Value ret = body.evaluate(ctx);
    ctx.popBlock();
    return ret;
}

Value InterpredText::evaluate(EvalContext& ctx) {
    auto desc = ctx.findSymbol(name);
    if (desc.ty == 2) {
        if (desc.value)
            return *desc.value;
    }
    if (desc.ty == 1) {
        auto* fn = desc.user_defined_fn;
        fn->args = args;
        return fn->evaluate(ctx);
    }
    if (desc.ty == 0 && desc.native_fn)
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
    EvalContext ctx {{
        std::shared_ptr<EvalFrame>(new EvalFrame {{
            {"!",        {0, false, 0, {&comment}}},
            {"date",     {0, true,  0, {&date}}},
            {"showif",   {1, false, 0, {&showif}}},
            {"math",     {1, true,  0, {&mexpr}}},
            {"if",       {1, false, 0, {&ifexpr}}},
            {"function", {1, false, 0, {&fnexpr}}},
            {"concat",   {0, false, 0, {&strconcat}}},
        }})
    }};
    std::cout << to_text(root->evaluate(ctx)) << "\n";
}
