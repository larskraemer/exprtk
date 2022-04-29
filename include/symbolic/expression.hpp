#pragma once

#include <array>
#include <limits>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include "math/rational.hpp"
#include "math/mpi.hpp"

namespace symb{
namespace impl{

template<class T, class U>
auto get_as(const std::unique_ptr<U>& ptr){
    return dynamic_cast<const T*>(ptr.get());
}

template<class T, class U>
auto get_as(std::unique_ptr<U>& ptr){
    return dynamic_cast<T*>(ptr.get());
}


template<std::ranges::range R>
auto to_vector(R&& r) {
    std::vector<std::ranges::range_value_t<R>> ret;
    for(auto x : r){
        ret.emplace_back(std::move(x));
    }   
    return ret;
}

template<class T, class... Ts>
auto to_vector(Ts&&... ts) {
    std::vector<T> ret;
    ret.reserve(sizeof...(Ts));
    (
        [&]<class U>(U&& u){
            ret.emplace_back(std::forward<U>(u));
        }(std::forward<Ts>(ts)), ...
    );
    return ret;
}


enum class Kind : int {
    Number = 0,
    ProdOp,
    PowOp,
    SumOp,
    Function,
    Symbol,
    Undefined
};


constexpr auto precedence(Kind k) noexcept {
    switch (k)
    {
    case Kind::SumOp: return 1;
    case Kind::ProdOp: return 2;
    case Kind::PowOp: return 3;
    default: return std::numeric_limits<int>::max();
    }
}

struct ExpressionBase;

using ExprPtr = std::unique_ptr<ExpressionBase>;

template<class T, class... Ts> requires std::derived_from<T, ExpressionBase>
ExprPtr make_expression(Ts&&... ts) {
    return std::make_unique<T>(std::forward<Ts>(ts)...);
}

struct ExpressionBase{
    using Number_t = FieldOfFractions<multiprecision::MPi>;

    explicit ExpressionBase(std::vector<ExprPtr> _children)
        : children(std::move(_children))
    {}

    ExpressionBase() : ExpressionBase(std::vector<ExprPtr>{}) {}

    virtual auto kind() const -> Kind = 0;
    virtual auto copy() const -> ExprPtr = 0;
    virtual auto str() const -> std::string = 0;
    virtual auto repr() const -> std::string = 0;

    virtual auto constant() const -> ExprPtr;
    virtual auto term() const -> ExprPtr;
    virtual auto base() const -> ExprPtr;
    virtual auto exponent() const -> ExprPtr;

    auto maybe_brace(const ExprPtr& x) const {
        if(precedence(x->kind()) < precedence(kind())){
            return "(" + x->str() + ")";
        }
        else{
            return x->str();
        }
    }

    virtual ~ExpressionBase(){}

    std::vector<ExprPtr> children;
};

struct Number : public ExpressionBase{
    template<class T> requires std::constructible_from<Number_t, T>
    explicit Number(T v) : value{Number_t{std::move(v)}} {}
    explicit Number(Number_t v) : value{v} {}
    virtual auto kind() const -> Kind override { return Kind::Number; }
    virtual auto copy() const -> ExprPtr override {
        return make_expression<Number>(value);
    }
    virtual auto str() const -> std::string override {
        using std::to_string;
        return to_string(value);
    }
    virtual auto repr() const -> std::string override {
        using std::to_string;
        return to_string(value);
    }
    Number_t value;
};

struct Symbol : public ExpressionBase{
    explicit Symbol(std::string n) : name{std::move(n)} {}
    virtual auto kind() const -> Kind override { return Kind::Symbol; }
    virtual auto copy() const -> ExprPtr override {
        return make_expression<Symbol>(name);
    }
    virtual auto str() const -> std::string override { return name; }
    virtual auto repr() const -> std::string override { return name; }
    std::string name;
};

struct Sum : public ExpressionBase{
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, ExprPtr>
    Sum(R init)
        : ExpressionBase(to_vector( init | std::views::transform([](const ExprPtr& x){ return x->copy(); }) ))
    {}
    
    Sum(ExprPtr x, ExprPtr y){
        children.emplace_back(std::move(x));
        children.emplace_back(std::move(y));
    }

    virtual auto kind() const -> Kind override { return Kind::SumOp; }
    virtual auto copy() const -> ExprPtr override {
        return make_expression<Sum>(std::views::all(children));
    }
    virtual auto str() const -> std::string override {
        std::string ret;
        for(const auto& y : children){
            if(ret.empty()) ret = maybe_brace(y);
            else{
                auto s = maybe_brace(y);
                if(s[0] == '-') ret += s;
                else ret += "+" + s;
            } 
        }
        return ret;
    }
    virtual auto repr() const -> std::string override {
        auto summands_str = std::string();
        for(auto& x : children){
            if(summands_str.empty()) summands_str += x->repr();
            else summands_str += ", " + x->repr();
        }
        return fmt::format("Sum({})", summands_str);
    }
};

struct Product : public ExpressionBase{
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, ExprPtr>
    Product(R init){
        for(const auto& x : init){
            children.emplace_back(x->copy());
        }
    }

    Product(ExprPtr x, ExprPtr y){
        children.emplace_back(std::move(x));
        children.emplace_back(std::move(y));
    }

    virtual auto kind() const -> Kind override { return Kind::ProdOp; }
    virtual auto copy() const -> ExprPtr override {
        return make_expression<Product>(std::views::all(children));
    }

    virtual auto constant() const -> ExprPtr override { 
        if(children[0]->kind() == Kind::Number){
            return children[0]->copy();
        }
        else{
            return make_expression<Number>(1);
        }
    }

    virtual auto term() const -> ExprPtr override {
        if(children[0]->kind() == Kind::Number){
            return make_expression<Product>(std::ranges::subrange(children.begin() + 1, children.end()));
        }
        else{
            return copy();
        }
    }

    virtual auto str() const -> std::string override {
        std::string ret;
        for(const auto& y : children){
            if(ret.empty()){
                if(y->kind() == Kind::Number && get_as<Number>(y)->value == -1){
                    ret = "-";
                }
                else{
                    ret = maybe_brace(y);
                }
            }
            else ret += "*" + maybe_brace(y);
        }
        return ret;
    }
    virtual auto repr() const -> std::string override {
        auto factors_str = std::string();
        for(auto& x : children){
            if(factors_str.empty()) factors_str += x->repr();
            else factors_str += ", " + x->repr();
        }
        return fmt::format("Product({})", factors_str);
    }
};

struct Power : public ExpressionBase{
    explicit Power(ExprPtr vbase, ExprPtr vexponent)
    {
        children.emplace_back(std::move(vbase));
        children.emplace_back(std::move(vexponent));
    }

    virtual auto kind() const -> Kind override { return Kind::PowOp; }
    virtual auto copy() const -> ExprPtr override {
        return make_expression<Power>(children[0]->copy(), children[1]->copy());
    }

    virtual auto base() const -> ExprPtr override { return children[0]->copy(); }
    virtual auto exponent() const -> ExprPtr override { return children[1]->copy(); }
    virtual auto str() const -> std::string override {
        return maybe_brace(children[0]) + "^" + maybe_brace(children[1]);
    }
    virtual auto repr() const -> std::string override {
        auto factors_str = std::string();
        for(auto& x : children){
            if(factors_str.empty()) factors_str += x->repr();
            else factors_str += ", " + x->repr();
        }
        return fmt::format("Power({})", factors_str);
    }
};

struct Function : public ExpressionBase {
    Function(std::string _name, ExprPtr argument)
        : ExpressionBase(), name{std::move(_name)}
    {
        children.emplace_back(std::move(argument));
    }

    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, ExprPtr>
    Function(std::string _name, R arguments)
        : ExpressionBase(to_vector(arguments | std::views::transform([](const ExprPtr& x){ return x->copy(); }))), name{std::move(_name)}
    {}

    template<class... Ts>
    Function(std::string _name, Ts&&... ts) 
        : ExpressionBase(to_vector<ExprPtr>(std::forward<Ts>(ts)...)), name{std::move(_name)}
    {}


    virtual auto kind() const -> Kind override { return Kind::Function; }
    virtual auto copy() const -> ExprPtr override { return make_expression<Function>(name, std::views::all(children)); }
    virtual auto str() const -> std::string override { 
        std::string arg_str;
        for(const auto& arg : children) {
            if(arg_str.empty()){
                arg_str = arg->str();
            }
            else{
                arg_str += ", " + arg->str();
            }
        }
        return fmt::format("{}({})", name, arg_str);
    }
    virtual auto repr() const -> std::string override {
        auto args_str = std::string();
        for(auto& x : children){
            if(args_str.empty()) args_str += x->repr();
            else args_str += ", " + x->repr();
        }
        return fmt::format("Function({})({})", name, args_str);
    }

    std::string name;
};

struct Undefined : public ExpressionBase{
    virtual auto kind() const -> Kind override { return Kind::Undefined; }
    virtual auto copy() const -> ExprPtr override { return make_expression<Undefined>(); }
    virtual auto str() const -> std::string override { return "<Undefined>"; }
    virtual auto repr() const -> std::string override { return "<Undefined>"; }
};


// Unpacks an expression val into a pair c, t such that:
// c is a number
// c*t == val
inline auto unpack_term(ExprPtr val) -> std::array<ExprPtr, 2> {
    if(val->kind() == Kind::ProdOp){
        if(val->children[0]->kind() == Kind::Number){
            auto c = std::move(val->children[0]);
            val->children.erase(val->children.begin());
            return {
                std::move(c),
                std::move(val)
            };
        }
        else{
            return {
                make_expression<Number>(1),
                std::move(val)
            };
        }
    }
    else{
        return {
            make_expression<Number>(1),
            std::move(val)
        };
    }
}

// Unpacks an expression val into a pair of expressions b,e such that
// b^e == val
inline auto unpack_power(ExprPtr val) -> std::array<ExprPtr, 2> {
    if(val->kind() == Kind::PowOp) return {
        std::move(val->children[0]),
        std::move(val->children[1])
    };
    else return {
        std::move(val),
        make_expression<Number>(1)
    };
}

} //namespace impl
} //namespace symb