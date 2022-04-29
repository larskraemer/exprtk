#pragma once

#include "expression.hpp"
#include "compare.hpp"


namespace symb{
namespace impl{

struct SimplificationContext;

struct SimplificationRule {
    using SimplifyTransform = ExprPtr(*)(const SimplificationContext&, ExprPtr);
    bool(*match)(const ExprPtr&);
    std::vector<SimplifyTransform> transforms;
};

struct SimplificationContext{
    bool is_number(const ExprPtr& t) const {
        return t->kind() == Kind::Number;
    }
    bool is_one(const ExprPtr& t) const {
        return is_number(t) && (get_as<Number>(t)->value == ExpressionBase::Number_t(1));
    }
    bool is_zero(const ExprPtr& t) const {
        auto ret = is_number(t) && (get_as<Number>(t)->value == ExpressionBase::Number_t(0));
        return ret;
    }
    bool is_integral(const ExprPtr& t) const {
        return is_number(t) && math::is_integer(get_as<Number>(t)->value);
    }
    bool is_constant(const ExprPtr& t, std::optional<std::vector<std::string>> variables) const {
        switch (t->kind())
        {
        case Kind::Number: return true;
        case Kind::Symbol:{
            if(variables){
                return std::all_of(variables->begin(), variables->end(), [&](const std::string& name){
                    return get_as<Symbol>(t)->name != name;
                });
            }
            else{
                return false;
            }
        }
        case Kind::Function:
        case Kind::SumOp:
        case Kind::ProdOp:
        case Kind::PowOp: {
            return std::all_of(t->children.begin(), t->children.end(), [&](const ExprPtr& x){
                return is_constant(x, variables);
            });
        }
        default: return false;
        }
    }

    std::vector<SimplificationRule> rules;
};



struct Simplifier{
    static ExprPtr automatic_simplify(ExprPtr);

    static ExprPtr automatic_simplify_impl(const SimplificationContext& sc, ExprPtr t);
    static ExprPtr automatic_simplify_product(const SimplificationContext& sc, ExprPtr t);
    static ExprPtr automatic_simplify_sum(const SimplificationContext& sc, ExprPtr t);
    static ExprPtr automatic_simplify_power(const SimplificationContext& sc, ExprPtr t);
    static ExprPtr automatic_simplify_function(const SimplificationContext&, ExprPtr);

    static ExprPtr automatic_simplify_integer_power(const SimplificationContext& sc, ExprPtr t);

    static void simplify_subexpressions(const SimplificationContext&, ExprPtr&, ExprPtr(*)(const SimplificationContext&, ExprPtr));
};


} //namespace impl
} //namespace symb