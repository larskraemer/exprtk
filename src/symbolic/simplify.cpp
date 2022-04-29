#include "symbolic/simplify.hpp"



namespace symb{
namespace impl{

template<Kind k>
auto assoc_expand(const SimplificationContext&, ExprPtr expr) -> ExprPtr{
    std::vector<ExprPtr> tmp;
    for(auto& subexpr : expr->children){
        if(subexpr->kind() == k)
            for(auto& factor : subexpr->children) tmp.emplace_back(std::move(factor));
        else
            tmp.emplace_back(std::move(subexpr));
    }
    std::swap(tmp, expr->children);
    return expr;
}

auto sort_subexpressions(const SimplificationContext&, ExprPtr t){
    std::sort(t->children.begin(), t->children.end(), [](const auto& lhs, const auto& rhs){ 
        return cmp_expression(lhs, rhs) < 0;
    });
    return t;
};

constexpr auto combine_subexpressions(ExprPtr expr, auto combine_fuc) {
    auto& children = expr->children;
    auto read_iter = children.begin();
    auto end_iter = children.begin();
    for(;read_iter != children.end(); ++read_iter) {
        if(end_iter == children.begin()){
            if(read_iter != end_iter){
                *end_iter = std::move(*read_iter);
            }
            ++end_iter;
        }
        else{
            end_iter = combine_fuc(end_iter - 1, *(end_iter - 1), *read_iter);
        }
    }
    children.resize(end_iter - children.begin());
    return expr;
};


ExprPtr Simplifier::automatic_simplify(ExprPtr x){ 
    auto sc = SimplificationContext{};

    return automatic_simplify_impl(sc, std::move(x));
}

ExprPtr Simplifier::automatic_simplify_impl(const SimplificationContext& sc, ExprPtr expr){
    simplify_subexpressions(sc, expr, automatic_simplify_impl);
    switch(expr->kind()){
    case Kind::Function: return automatic_simplify_function(sc, std::move(expr));
    case Kind::PowOp : return automatic_simplify_power(sc, std::move(expr));
    case Kind::ProdOp : return automatic_simplify_product(sc, std::move(expr));
    case Kind::SumOp : return automatic_simplify_sum(sc, std::move(expr));
    default: return expr;
    }
}

void Simplifier::simplify_subexpressions(const SimplificationContext& sc, ExprPtr& expr, ExprPtr (*simplify_func)(const SimplificationContext&,ExprPtr)){
    for(auto& x : expr->children){
        x = simplify_func(sc, std::move(x));
    }
}

ExprPtr Simplifier::automatic_simplify_sum(const SimplificationContext& sc, ExprPtr expr){
    expr = assoc_expand<Kind::SumOp>(sc, std::move(expr));
    expr = sort_subexpressions(sc, std::move(expr));
    expr = combine_subexpressions(std::move(expr), [&](auto write_iter, ExprPtr& lhs, ExprPtr& rhs){
        auto unpack_number = [](const ExprPtr& ptr){
            return get_as<Number>(ptr)->value;
        };
        if(sc.is_number(lhs) && sc.is_number(rhs)){
            auto v = unpack_number(lhs) + unpack_number(rhs);
            if(v != 0){
                *write_iter = make_expression<Number>(v);
                ++write_iter;
            }
        }
        else if(sc.is_zero(lhs)){
            *write_iter = std::move(rhs);
            ++write_iter;
        }
        else if(sc.is_zero(rhs)){
            *write_iter = std::move(lhs);
            ++write_iter;
        }
        else if(cmp_expression( lhs->term(), rhs->term() ) == 0){
            // Combine like terms

            auto[lc, lt] = unpack_term(std::move(lhs));
            auto[rc, rt] = unpack_term(std::move(rhs));

            auto new_constant = automatic_simplify_sum(sc, make_expression<Sum>(
                std::move(lc), std::move(rc)
            ));
            auto new_factor = automatic_simplify_product(sc, make_expression<Product>(
                std::move(new_constant), std::move(lt)
            ));
            if(not sc.is_zero(new_factor)){
                *write_iter = std::move(new_factor);
                ++write_iter;
            }
        }
        else{
            *write_iter = std::move(lhs);
            ++write_iter;
            *write_iter = std::move(rhs);
            ++write_iter;
        }
        return write_iter;
    });
    if(expr->children.size() == 0) return make_expression<Number>(0);
    else if(expr->children.size() == 1) return std::move(expr->children[0]);
    else return expr;
}

ExprPtr Simplifier::automatic_simplify_product(const SimplificationContext& sc, ExprPtr expr){
    expr = assoc_expand<Kind::ProdOp>(sc, std::move(expr));
    expr = sort_subexpressions(sc, std::move(expr));
    expr = combine_subexpressions(std::move(expr), [&](auto write_iter, ExprPtr& lhs, ExprPtr& rhs){
        auto unpack_number = [](const ExprPtr& num){
            return get_as<Number>(num)->value;
        };
        if(sc.is_number(lhs) && sc.is_number(rhs)){
            auto v = unpack_number(lhs) * unpack_number(rhs);
            if(v != 1){
                *write_iter = make_expression<Number>(v);
                ++write_iter;
            }
        }
        else if(sc.is_one(lhs)){
            *write_iter = std::move(rhs);
            ++write_iter;
        }
        else if(sc.is_one(rhs)){
            *write_iter = std::move(lhs);
            ++write_iter;
        }
        else if(cmp_base(lhs, rhs) == 0){
            // Combine exponents
            auto[lb, le] = unpack_power(std::move(lhs));
            auto[rb, re] = unpack_power(std::move(rhs));

            auto new_exponent = automatic_simplify_sum(sc, make_expression<Sum>(
                std::move(le), std::move(re)
            ));
            auto new_factor = automatic_simplify_power(sc, make_expression<Power>(
                std::move(lb), std::move(new_exponent)
            ));
            if(not sc.is_one(new_factor)){
                *write_iter = std::move(new_factor);
                ++write_iter;
            }
        }
        else{
            *write_iter = std::move(lhs);
            ++write_iter;
            *write_iter = std::move(rhs);
            ++write_iter;
        }
        return write_iter;
    });
    if(expr->children.size() == 0) return make_expression<Number>(1);
    else if(expr->children.size() == 1) return std::move(expr->children[0]);
    else return expr;
}

ExprPtr Simplifier::automatic_simplify_integer_power(const SimplificationContext& sc, ExprPtr t) {
    auto b = std::move(t->children[0]);
    auto e = std::move(t->children[1]);

    if(sc.is_zero(e)) return make_expression<Number>(1);
    if(sc.is_one(e)) return b;

    if(b->kind() == Kind::Number) {
        return make_expression<Number>(
            math::pow(
                get_as<Number>(b)->value,
                get_as<Number>(e)->value
            )
        );
    }
    if(b->kind() == Kind::PowOp){
        auto new_exponent = automatic_simplify_product(sc,
            make_expression<Product>(
                std::move(b->children[0]),
                std::move(e)
            )
        );
        return automatic_simplify_power(sc,
            make_expression<Power>(
                std::move(b->children[1]),
                std::move(new_exponent)
            )
        );
    }
    if(b->kind() == Kind::ProdOp){
        auto ptr = get_as<Product>(b);
        return automatic_simplify_product(sc,
            make_expression<Product>(
                std::views::all(ptr->children) | std::views::transform([&](auto& term){
                    return automatic_simplify_power(sc, make_expression<Power>(
                        std::move(term),
                        e->copy()
                    ));
                })
            )
        );
    }

    return make_expression<Power>(std::move(b), std::move(e));
}

ExprPtr Simplifier::automatic_simplify_power(const SimplificationContext& sc, ExprPtr t) {
    auto& b = t->children[0];
    auto& e = t->children[1];

    if(sc.is_zero(b)){
        if(e->kind() == Kind::Number){
            auto v = get_as<Number>(e)->value;
            if(v > 0) return make_expression<Number>(0);
            if(v == 0) return make_expression<Number>(1);
            return make_expression<Undefined>();
        }
        else{
            return t;
        }
    }
    else if(sc.is_one(b)){
        return make_expression<Number>(1);
    }
    else if(sc.is_integral(e)) {
        return automatic_simplify_integer_power(sc, std::move(t));
    }
    else{
        return t;
    }
}

auto simplify_differentiation(const SimplificationContext& sc, ExprPtr x) -> ExprPtr {
    using std::views::all;
    using std::views::transform;

    if(x->children.size() != 2){
        throw std::runtime_error(fmt::format(
            "Invalid function call: {}", x->str()
        ));
    }

    auto expr = std::move(x->children[0]);
    auto var = std::move(x->children[1]);

    if(var->kind() != Kind::Symbol){
        throw std::runtime_error(fmt::format(
            "Invalid differentiation variable: {}", var->str()
        ));
    }
    switch(expr->kind()){
    case Kind::Symbol:{
        if(cmp_expression(expr, var) == 0){
            return make_expression<Number>(1);
        }
        else{
            return make_expression<Number>(0);
        }
    }
    case Kind::Number:{
        return make_expression<Number>(0);
    }
    case Kind::PowOp:{
        auto [base, exp] = unpack_power(std::move(expr));
        if(sc.is_constant(exp, {{get_as<Symbol>(var)->name}})){
            std::vector<ExprPtr> factors;
            factors.emplace_back(exp->copy());
            factors.emplace_back(
                Simplifier::automatic_simplify_power(
                    sc, 
                    make_expression<Power>(
                        base->copy(),
                        Simplifier::automatic_simplify_sum(
                            sc, 
                            make_expression<Sum>(
                                exp->copy(),
                                make_expression<Number>(-1)
                            )
                        )
                    )
                )
            );
            factors.emplace_back(
                Simplifier::automatic_simplify_function(
                    sc, 
                    make_expression<Function>(
                        "diff",
                        base->copy(),
                        var->copy()
                    )
                )
            );
            return Simplifier::automatic_simplify_product(
                sc,
                make_expression<Product>(
                    std::move(factors)
                )
            ); 
        }
        else{
            throw std::runtime_error(fmt::format(
                "Differentiation of non-constant exponent is not yet implemented: {}", exp->str()
            ));
        }
    }
    case Kind::ProdOp:{
        auto& factors = expr->children;
        for(auto& f : factors) fmt::print("{}, ", f->repr());
        fmt::print("\n");

        std::vector<ExprPtr> summands;
        for(size_t factor_to_diff = 0; factor_to_diff < factors.size(); factor_to_diff++){
            std::vector<ExprPtr> new_factors;
            for(size_t i = 0; i < factors.size(); i++){
                if(i == factor_to_diff){
                    new_factors.emplace_back(
                        Simplifier::automatic_simplify_function(
                            sc,
                            make_expression<Function>(
                                "diff", 
                                factors[i]->copy(), 
                                var->copy()
                            )
                        )
                    );
                }
                else{
                    new_factors.emplace_back(factors[i]->copy());
                }
            }
            summands.emplace_back(
                Simplifier::automatic_simplify_product(
                    sc,
                    make_expression<Product>(
                        std::move(new_factors)
                    )
                )
            );
        }
        return Simplifier::automatic_simplify_sum(
            sc,
            make_expression<Sum>(
                std::move(summands)
            )
        );
    }
    case Kind::SumOp:{
        std::vector<ExprPtr> new_summands;
        for(auto& s : expr->children){
            new_summands.emplace_back(
                make_expression<Function>("diff", std::move(s), var->copy())
            );
        }
        return Simplifier::automatic_simplify_sum(
            sc,
            make_expression<Sum>(
                std::move(new_summands)
            )
        );
    }
    case Kind::Function:{
        return make_expression<Function>("diff", std::move(expr), std::move(var));
    }
    default: return make_expression<Undefined>();
    }
}

auto Simplifier::automatic_simplify_function(const SimplificationContext& sc, ExprPtr x) -> ExprPtr {
    auto ptr = get_as<Function>(x);
    if(ptr->name == "diff"){
        auto before = x->repr();
        auto ret = simplify_differentiation(sc, std::move(x));
        return ret;
    }
    else{
        return x;
    }
}

}
}