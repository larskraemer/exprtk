#include "symbolic/expression.hpp"
#include "symbolic/compare.hpp"


namespace symb{
namespace impl{
    

std::strong_ordering cmp_expression(const ExprPtr& lhs, const ExprPtr& rhs) {
    using std::views::single;
    using std::views::all;
    using std::views::transform;

    if(cmp_kind(lhs->kind(), rhs->kind()) > 0){
        auto c = cmp_expression(rhs, lhs);
        if(c < 0) return std::strong_ordering::greater;
        if(c == 0) return std::strong_ordering::equal;
        if(c > 0) return std::strong_ordering::less;
    }

    switch (lhs->kind())
    {
    case Kind::Number:{
        if(rhs->kind() == Kind::Number)
            return get_as<Number>(lhs)->value <=> get_as<Number>(rhs)->value;
        else
            return std::strong_ordering::less;
    }
    case Kind::ProdOp:{
        if(rhs->kind() == Kind::ProdOp)
            return cmp_expression_list(all(lhs->children), all(rhs->children));
        else
            return cmp_expression_list(all(lhs->children), single_cref_view(rhs));
    }
    case Kind::PowOp:{
        if(rhs->kind() == Kind::PowOp){
            auto c = cmp_expression(lhs->children[0], rhs->children[0]);
            if(c == 0) return cmp_expression(lhs->children[1], rhs->children[1]);
            else return c;
        }
        else{
            auto c = cmp_expression(lhs->children[0], rhs);
            if(c == 0) return cmp_expression(lhs->children[1], make_expression<Number>(1));
            else return c;
        }
    }
    case Kind::SumOp:{
        if(rhs->kind() == Kind::SumOp)
            return cmp_expression_list(all(lhs->children), all(rhs->children));
        else
            return cmp_expression_list(all(lhs->children), single_cref_view(rhs));
    }
    case Kind::Function:{
        if(rhs->kind() == Kind::Function){
            auto c = get_as<Function>(lhs)->name <=> get_as<Function>(rhs)->name;
            if(c == 0)
                return cmp_expression_list(all(lhs->children), all(rhs->children));
            else return c;
        }
        else{
            return cmp_expression_list(all(lhs->children), single_cref_view(rhs));
        }
    }
    case Kind::Symbol:{
        if(rhs->kind() == Kind::Symbol)
            return get_as<Symbol>(lhs)->name <=> get_as<Symbol>(rhs)->name;
        else
            return std::strong_ordering::less;
    }
    case Kind::Undefined:{
        if(rhs->kind() == Kind::Undefined) return std::strong_ordering::equal;
        return std::strong_ordering::less;
    }
    }
    throw std::runtime_error(fmt::format("Unknown Expression kind: {}", static_cast<int>(lhs->kind())));
}

} // namespace impl
} // namespace symb