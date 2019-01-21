#ifndef AST
#define AST

#include <string>
#include <vector>
#include <map>
#include <stack>
#include <memory>
#include <utility>
#include <iostream>
#include <algorithm>
#include <iterator>


// #define LITERAL 2
// #define IDENT  3
// #define NUMBER 4
// #define STRING 5
// #define OBRACK 8
// #define CBRACK 9

extern "C" int yyparse();

typedef struct {
    struct {
        int tag : 2;
        int literal : 1;
    } info;
    union {
        char* text;
        int number;
    };
} Value;

class ExprNode;
class EvalContext;
class BlockExprNode;

Value dummy_inner_ty(int, std::vector<Value*>, BlockExprNode, EvalContext&);
typedef decltype(dummy_inner_ty) lib_func_type;

typedef struct {
    int argument_count;
    bool needs_end;
    lib_func_type* native_fn;
} TagDescriptor;

class EvalContext {
public:
    std::map<std::string, TagDescriptor> tags;
    std::stack<EvalContext*> blocks;
};

class ASTNode {
public:
    ASTNode() {};
    virtual ~ASTNode(){};
    virtual Value evaluate(EvalContext&) = 0;
    virtual int ty() = 0;
};
class ExprNode: public ASTNode {};

class BlockExprNode: public ExprNode {
public:
    std::vector<ExprNode*> code;
    bool parsed;
    BlockExprNode(std::vector<ExprNode*> v) : code(v), parsed(false) {}
    BlockExprNode() : code (std::vector<ExprNode*>()), parsed(false) {}

    virtual Value evaluate(EvalContext&);
    virtual int ty() { return 0; }
};

class InterpredText: public ExprNode {
public:
    std::string name;
    std::vector<ExprNode*> args;
    BlockExprNode contained;
    using InsideItType = decltype(contained.code)::iterator;
    using ReverseInsideItType = decltype(contained.code)::reverse_iterator;

    InterpredText(std::string n, std::vector<ExprNode*> a) : name(n), args(a), contained() {}

    virtual Value evaluate(EvalContext&);
    virtual int ty() { return 1; }
    void parseForward(InsideItType&, InsideItType&, EvalContext&);
    void parseForward(ReverseInsideItType&, ReverseInsideItType&, EvalContext&);
};

class LiteralText: public ExprNode {
public:
    std::string value;

    LiteralText(std::string n) : value(n) {}

    virtual Value evaluate(EvalContext&);
    virtual int ty() { return 2; }
};

struct sequence_tag {};
struct pointer_tag {};

template< class X >
X category( ... );

template< class S >
auto category( const S& s ) -> decltype( std::begin(s), sequence_tag() );

template< class Ptr >
auto category( const Ptr& p ) -> decltype( *p, p==nullptr, pointer_tag() );

template< class T > struct Category {
    using type = decltype( category<T>(std::declval<T>()) );
};

template< class R, class ... X > struct Category< R(&)(X...) > {
    using type = R(&)(X...);
};

template< class T >
using Cat = typename Category<T>::type;

template< class... > struct Functor;

template< class F, class FX, class Fun=Functor< Cat<FX> > >
auto fmap( F&& f, FX&& fx )
    -> decltype( Fun::fmap( std::declval<F>(), std::declval<FX>() ) )
{
    return Fun::fmap( std::forward<F>(f), std::forward<FX>(fx) );
}

template< class F, class G >
struct Composition {
    F f;
    G g;

    template< class X >
    auto operator () ( X&& x ) -> decltype( f(g(std::declval<X>())) ) {
        return f(g(std::forward<X>(x)));
    }
};

// General case: composition
template< class Function > struct Functor<Function> {
    template< class F, class G, class C = Composition<F,G> >
    static C fmap( F f, G g ) {
        C( std::move(f), std::move(g) );
    }
};

template<> struct Functor< sequence_tag > {
    template< class F, template<class...>class S, class X,
              class R = typename std::result_of<F(X)>::type >
    static S<R> fmap( F&& f, const S<X>& s ) {
        S<R> r;
        r.reserve( s.size() );
        std::transform( std::begin(s), std::end(s),
                        std::back_inserter(r),
                        std::forward<F>(f) );
        return r;
    }
};

template<> struct Functor< pointer_tag > {
    template< class F, template<class...>class Ptr, class X,
              class R = typename std::result_of<F(X)>::type >
    static Ptr<R> fmap( F&& f, const Ptr<X>& p )
    {
        return p != nullptr
            ? Ptr<R>( new R( std::forward<F>(f)(*p) ) )
            : nullptr;
    }
};

#endif /* end of include guard: AST */
