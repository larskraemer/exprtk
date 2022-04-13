#pragma once

#include <array>
#include <limits>
#include <ranges>
#include <string>
#include <variant>
#include <vector>

#include "rational.hpp"
#include "overloaded.hpp"
#include "single_ref_view.hpp"

enum class Kind : int {
    Number = 0,
    ProdOp,
    PowOp,
    SumOp,
    Symbol,
    Undefined
};

constexpr int compare_order(Kind k){
    return static_cast<int>(k);
}

constexpr auto precedence(Kind k) noexcept {
    switch (k)
    {
    case Kind::SumOp: return 1;
    case Kind::ProdOp: return 2;
    case Kind::PowOp: return 3;
    default: return std::numeric_limits<int>::max();
    }
}

struct ExpressionBase{
    using Number_t = FieldOfFractions<int>;
    virtual auto kind() const -> Kind = 0;
    virtual auto copy() const -> std::unique_ptr<ExpressionBase> = 0;
};

using ExprPtr = std::unique_ptr<ExpressionBase>;


struct Number : public ExpressionBase{

    explicit Number(Number_t v) : value{v} {}
    virtual auto kind() const -> Kind override { return Kind::Number; }
    virtual auto copy() const -> ExprPtr override {
        return std::make_unique<Number>(value);
    }
    Number_t value;
};

struct Symbol : public ExpressionBase{
    explicit Symbol(std::string n) : name{std::move(n)} {}
    virtual auto kind() const -> Kind override { return Kind::Symbol; }
    virtual auto copy() const -> ExprPtr override {
        return std::make_unique<Symbol>(name);
    }
    std::string name;
};

struct Sum : public ExpressionBase{
    template<std::ranges::range R>
    requires std::convertible_to<std::ranges::range_value_t<R>, ExprPtr>
    Sum(R init){
        for(const auto& x : init){
            children.emplace_back(x->copy());
        }
    }
    Sum(ExprPtr x, ExprPtr y){
        children.emplace_back(std::move(x));
        children.emplace_back(std::move(y));
    }

    virtual auto kind() const -> Kind override { return Kind::SumOp; }
    virtual auto copy() const -> ExprPtr override {
        return std::make_unique<Sum>(std::views::all(children));
    }
    std::vector<ExprPtr> children;
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
        return std::make_unique<Product>(std::views::all(children));
    }
    std::vector<ExprPtr> children;
};

struct Power : public ExpressionBase{
    explicit Power(ExprPtr vbase, ExprPtr vexponent)
        : base{std::move(vbase)}, exponent{std::move(vexponent)}
    {}

    virtual auto kind() const -> Kind override { return Kind::PowOp; }
    virtual auto copy() const -> ExprPtr override {
        return std::make_unique<Power>(base->copy(), exponent->copy());
    }

    ExprPtr base;
    ExprPtr exponent;
};

struct Undefined : public ExpressionBase{
    virtual auto kind() const -> Kind override { return Kind::Undefined; }
    virtual auto copy() const -> ExprPtr override { return std::make_unique<Undefined>(); }
};

template<class T, class U>
auto get_as(const std::unique_ptr<U>& ptr){
    return reinterpret_cast<const T*>(ptr.get());
}

template<class T, class U>
auto get_as(std::unique_ptr<U>& ptr){
    return reinterpret_cast<T*>(ptr.get());
}

ExprPtr base(const ExprPtr& t) {
    if(t->kind() == Kind::PowOp){
        return reinterpret_cast<const Power*>(t.get())->base->copy();
    }
    else{
        return t->copy();
    }
}

ExprPtr exponent(const ExprPtr& t){
    if(t->kind() == Kind::PowOp){
        return reinterpret_cast<const Power*>(t.get())->exponent->copy();
    }
    else{
        return t->copy();
    }
}

ExprPtr term(const ExprPtr& t){
    switch(t->kind()){
    case Kind::ProdOp: {
        auto ptr = reinterpret_cast<const Product*>(t.get());
        if(ptr->children[0]->kind() == Kind::Number){
            return std::make_unique<Product>(
                std::ranges::subrange(ptr->children.begin() + 1, ptr->children.end())
            );
        }
        else{
            return ptr->copy();
        }
    } break;
    case Kind::Number: {
        return std::make_unique<Undefined>();
    } break;
    default: {
        return t->copy();
    } break;
    }
}

ExprPtr constant(const ExprPtr& t) {
    switch(t->kind()){
    case Kind::ProdOp: {
        auto ptr = reinterpret_cast<const Product*>(t.get());
        if(ptr->children[0]->kind() == Kind::Number){
            return ptr->children[0]->copy();
        }
        else{
            return std::make_unique<Number>(1);
        }
    } break;
    case Kind::Number: {
        return std::make_unique<Undefined>();
    } break;
    default: {
        return std::make_unique<Number>(1);
    } break;
    }
}

std::string expr_to_str(const ExprPtr& e) {
    auto maybe_brace = [&](const auto& x) -> std::string {
        if(precedence(x->kind()) < precedence(e->kind())){
            return "(" + expr_to_str(x) + ")";
        }
        else{
            return expr_to_str(x);
        }
    };

    using std::to_string;

    switch (e->kind())
    {
    case Kind::Number: return to_string(get_as<Number>(e)->value);
    case Kind::Symbol: return get_as<Symbol>(e)->name;
    case Kind::PowOp: return maybe_brace(get_as<Power>(e)->base) + "^" + maybe_brace(get_as<Power>(e)->exponent);
    case Kind::ProdOp: {
        auto ptr = get_as<Product>(e);
        std::string ret;
        for(const auto& y : ptr->children){
            if(ret.empty()) ret = maybe_brace(y);
            else ret += "*" + maybe_brace(y);
        }
        return ret;
    } break;
    case Kind::SumOp: {
        auto ptr = get_as<Sum>(e);
        std::string ret;
        for(const auto& y : ptr->children){
            if(ret.empty()) ret = maybe_brace(y);
            else ret += "+" + maybe_brace(y);
        }
        return ret;
    } break;
    case Kind::Undefined: return "<Undefined>";
    }
}

template<std::ranges::range A, std::ranges::range B>
std::strong_ordering cmp_expression_list(A lhs, B rhs);
std::strong_ordering cmp_expression(const ExprPtr& lhs, const ExprPtr& rhs);

std::strong_ordering cmp_expression(const ExprPtr& lhs, const ExprPtr& rhs) {

    using std::views::single;
    using std::views::all;
    using std::views::transform;

    if(compare_order(lhs->kind()) > compare_order(rhs->kind())){
        auto c = cmp_expression(rhs, lhs);
        if(c < 0) return std::strong_ordering::greater;
        if(c == 0) return std::strong_ordering::equal;
        if(c > 0) return std::strong_ordering::less;
    }

    if(lhs->kind() == Kind::Number){
        if(rhs->kind() == Kind::Number)
            return get_as<Number>(lhs)->value <=> get_as<Number>(rhs)->value;
        else
            return std::strong_ordering::less;
    }
    else if(lhs->kind() == Kind::ProdOp){
        if(rhs->kind() == Kind::ProdOp)
            return cmp_expression_list(all(get_as<Product>(lhs)->children), all(get_as<Product>(rhs)->children));
        else
            return cmp_expression_list(all(get_as<Product>(lhs)->children), single_cref_view(rhs));
    }
    else if(lhs->kind() == Kind::PowOp){
        if(rhs->kind() == Kind::PowOp){
            auto c = cmp_expression(get_as<Power>(lhs)->base, get_as<Power>(rhs)->base);
            if(c == 0) return cmp_expression(get_as<Power>(lhs)->exponent, get_as<Power>(rhs)->exponent);
            else return c;
        }
        else{
            auto c = cmp_expression(get_as<Power>(lhs)->base, rhs);
            if(c == 0) return cmp_expression(get_as<Power>(lhs)->exponent, exponent(rhs));
            else return c;
        }
    }
    else if(lhs->kind() == Kind::SumOp){
        if(rhs->kind() == Kind::SumOp)
            return cmp_expression_list(all(get_as<Sum>(lhs)->children), all(get_as<Sum>(rhs)->children));
        else
            return cmp_expression_list(all(get_as<Sum>(lhs)->children), single_cref_view(rhs));
    }
    else if(lhs->kind() == Kind::Symbol) {
        if(rhs->kind() == Kind::Symbol)
            return get_as<Symbol>(lhs)->name <=> get_as<Symbol>(rhs)->name;
        else
            return std::strong_ordering::less;
    }

    throw std::runtime_error("Compare overrun");
}

template<std::ranges::range A, std::ranges::range B>
std::strong_ordering cmp_expression_list(A lhs, B rhs){
    auto N = std::min(lhs.size(), rhs.size());
    for(auto k = 0u; k < N; k++){
        auto v = cmp_expression(lhs[lhs.size() - k - 1], rhs[rhs.size() - k - 1]);
        if(v != 0) return v;
    }
    return lhs.size() <=> rhs.size();
}

struct SimplificationContext{
    bool is_number(const ExprPtr& t) const {
        return t->kind() == Kind::Number;
    }
    bool is_one(const ExprPtr& t) const {
        return is_number(t) && (get_as<Number>(t)->value == 1);
    }
    bool is_zero(const ExprPtr& t) const {
        return is_number(t) && (get_as<Number>(t)->value == 0);
    }
    bool is_integral(const ExprPtr& t) const {
        return is_number(t) && math::is_integer(get_as<Number>(t)->value);
    }
};

ExprPtr automatic_simplify(const SimplificationContext& sc, ExprPtr t);
ExprPtr automatic_simplify_power(const SimplificationContext& sc, ExprPtr t);
ExprPtr automatic_simplify_integer_power(const SimplificationContext& sc, ExprPtr t);
ExprPtr automatic_simplify_product(const SimplificationContext& sc, ExprPtr t);
ExprPtr automatic_simplify_sum(const SimplificationContext& sc, ExprPtr t);

ExprPtr automatic_simplify_integer_power(const SimplificationContext& sc, ExprPtr t) {
    auto b = std::move(get_as<Power>(t)->base);
    auto e = std::move(get_as<Power>(t)->exponent);

    if(sc.is_zero(e)) return std::make_unique<Number>(1);
    if(sc.is_one(e)) return b;

    if(b->kind() == Kind::Number) {
        return std::make_unique<Number>(
            math::pow(
                get_as<Number>(b)->value,
                get_as<Number>(e)->value
            )
        );
    }
    if(b->kind() == Kind::PowOp){
        auto new_exponent = automatic_simplify_product(sc,
            std::make_unique<Product>(
                std::move(get_as<Power>(b)->exponent),
                std::move(e)
            )
        );
        return automatic_simplify_power(sc,
            std::make_unique<Power>(
                std::move(get_as<Power>(b)->base),
                std::move(new_exponent)
            )
        );
    }
    if(b->kind() == Kind::ProdOp){
        auto ptr = get_as<Product>(b);
        return automatic_simplify_product(sc,
            std::make_unique<Product>(
                std::views::all(ptr->children) | std::views::transform([&](auto& term){
                    return automatic_simplify_power(sc, std::make_unique<Power>(
                        std::move(term),
                        e->copy()
                    ));
                })
            )
        );
    }

    return std::make_unique<Power>(std::move(b), std::move(e));
}

ExprPtr automatic_simplify_power(const SimplificationContext& sc, ExprPtr t) {
    auto& b = get_as<Power>(t)->base;
    auto& e = get_as<Power>(t)->exponent;

    if(sc.is_zero(b)){
        if(e->kind() == Kind::Number){
            auto v = get_as<Number>(e)->value;
            if(v > 0) return std::make_unique<Number>(0);
            if(v == 0) return std::make_unique<Number>(1);
            return std::make_unique<Undefined>();
        }
        else{
            return t;
        }
    }
    else if(sc.is_one(b)){
        return std::make_unique<Number>(1);
    }
    else if(sc.is_integral(e)) {
        return automatic_simplify_integer_power(sc, std::move(t));
    }
    else{
        return t;
    }
}

ExprPtr automatic_simplify_product(const SimplificationContext& sc, ExprPtr t) {
    auto ptr = get_as<Product>(t);
    auto& children = ptr->children;

    if(children.size() == 1) return std::move(children[0]);

    // All children must already be fully simplified.
    auto assoc_expand_product = [](Product* p) {
        std::vector<std::unique_ptr<ExpressionBase>> tmp_storage;
        for(auto& factor : p->children){
            if(factor->kind() == Kind::ProdOp){
                auto& new_factors = get_as<Product>(factor)->children;
                for(auto& f : new_factors){
                    tmp_storage.emplace_back(std::move(f));
                }
            }
            else{
                tmp_storage.emplace_back(std::move(factor));
            }
        }

        std::swap(tmp_storage, p->children);
    };

    assoc_expand_product(ptr);
    if(std::any_of(children.begin(), children.end(), [&](const auto& x){ return sc.is_zero(x); })){
        return std::make_unique<Number>(0);
    }
    std::sort(children.begin(), children.end(), [](const auto& lhs, const auto& rhs){ 
        return cmp_expression(lhs, rhs) < 0;
    });

    // write the result of combining lhs and rhs to write_iter
    // return a new iterator past the end of the inserted terms
    // lhs may alias with *write_iter!
    auto combine_factors = [&](auto write_iter, ExprPtr& lhs, ExprPtr& rhs){
        auto unpack_number = [](const ExprPtr& ptr){
            return get_as<Number>(ptr)->value;
        };
        if(sc.is_number(lhs) && sc.is_number(rhs)){
            auto v = unpack_number(lhs) * unpack_number(rhs);
            if(v != 1){
                *write_iter = std::make_unique<Number>(v);
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
        else if(cmp_expression( base(lhs), base(rhs) ) == 0){
            // Combine exponents
            auto new_exponent = automatic_simplify_sum(sc, std::make_unique<Sum>(
                exponent(lhs), exponent(rhs)
            ));
            auto new_factor = automatic_simplify_power(sc, std::make_unique<Power>(
                base(lhs), std::move(new_exponent)
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
    };

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
            end_iter = combine_factors(end_iter - 1, *(end_iter - 1), *read_iter);
        }
    }
    children.resize(end_iter - children.begin());

    if(children.size() == 0) return std::make_unique<Number>(1);
    else if(children.size() == 1) return std::move(children[0]);
    else return t;
}

ExprPtr automatic_simplify_sum(const SimplificationContext& sc, ExprPtr t) {
    auto ptr = get_as<Sum>(t);
    auto& children = ptr->children;

    if(children.size() == 1) return std::move(children[0]);

    // All children must already be fully simplified.
    auto assoc_expand_product = [](Sum* p) {
        std::vector<ExprPtr> tmp_storage;
        for(auto& factor : p->children){
            if(factor->kind() == Kind::SumOp){
                auto& new_factors = get_as<Sum>(factor)->children;
                for(auto& f : new_factors){
                    tmp_storage.emplace_back(std::move(f));
                }
            }
            else{
                tmp_storage.emplace_back(std::move(factor));
            }
        }

        std::swap(tmp_storage, p->children);
    };

    assoc_expand_product(ptr);
    std::sort(children.begin(), children.end(), [](const auto& lhs, const auto& rhs){ 
        return cmp_expression(lhs, rhs) < 0;
    });

    // write the result of combining lhs and rhs to write_iter
    // return a new iterator past the end of the inserted terms
    // lhs may alias with *write_iter!
    auto combine_factors = [&](auto write_iter, ExprPtr& lhs, ExprPtr& rhs){
        auto unpack_number = [](const ExprPtr& ptr){
            return get_as<Number>(ptr)->value;
        };
        if(sc.is_number(lhs) && sc.is_number(rhs)){
            auto v = unpack_number(lhs) + unpack_number(rhs);
            if(v != 0){
                *write_iter = std::make_unique<Number>(v);
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
        else if(cmp_expression( term(lhs), term(rhs) ) == 0){
            // Combine like terms
            auto new_constant = automatic_simplify_sum(sc, std::make_unique<Sum>(
                constant(lhs), constant(rhs)
            ));
            auto new_factor = automatic_simplify_product(sc, std::make_unique<Product>(
                std::move(new_constant), term(lhs)
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
    };

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
            end_iter = combine_factors(end_iter - 1, *(end_iter - 1), *read_iter);
        }
    }
    children.resize(end_iter - children.begin());

    if(children.size() == 0) return std::make_unique<Number>(0);
    else if(children.size() == 1) return std::move(children[0]);
    else return t;
}

ExprPtr automatic_simplify(const SimplificationContext& sc, ExprPtr t) {
    switch(t->kind()){
    case Kind::Number: return t;
    case Kind::Symbol: return t;
    case Kind::PowOp: {
        auto ptr = get_as<Power>(t);
        ptr->base = automatic_simplify(sc, std::move(ptr->base));
        ptr->exponent = automatic_simplify(sc, std::move(ptr->exponent));
        return automatic_simplify_power(sc, std::move(t));
    }
    case Kind::ProdOp: {
        auto ptr = get_as<Product>(t);
        for(auto& x : ptr->children){
            x = automatic_simplify(sc, std::move(x));
        }
        return automatic_simplify_product(sc, std::move(t));
    }
    case Kind::SumOp: {
        auto ptr = get_as<Sum>(t);
        for(auto& x : ptr->children){
            x = automatic_simplify(sc, std::move(x));
        }
        return automatic_simplify_sum(sc, std::move(t));
    }
    default: return std::make_unique<Undefined>();
    }
}

ExprPtr simplify_subexpression(const SimplificationContext& sc, ExprPtr x){
    switch(x->kind()){
    case Kind::PowOp: {
        auto ptr = get_as<Power>(x);
        ptr->base = automatic_simplify(sc, std::move(ptr->base));
        ptr->exponent = automatic_simplify(sc, std::move(ptr->exponent));
    }
    case Kind::ProdOp: {
        auto ptr = get_as<Product>(x);
        for(auto& c : ptr->children){
            c = automatic_simplify(sc, std::move(x));
        }
        return x;
    }
    case Kind::SumOp: {
        auto ptr = get_as<Sum>(x);
        for(auto& c : ptr->children){
            c = automatic_simplify(sc, std::move(x));
        }
        return x;
    }
    }
    return x;
}


struct SimplificationRule {
    bool(*match)(const ExprPtr&) = [](const ExprPtr&){ return true; };
    ExprPtr(*transform)(const SimplificationContext&, ExprPtr) = [](const SimplificationContext&, ExprPtr x){ return x; };
};

template<Kind k>
constexpr auto match_type = [](const ExprPtr& ptr){
    return ptr->kind() == k;
};

constexpr auto match_any = [](const ExprPtr&){ return true; }

constexpr auto simplification_rules = std::array{
    SimplificationRule{ match_any, automatic_simplify },
    SimplificationRule{ match_type<Power>, automatic_simplify_power },
    SimplificationRule{ match_type<Product>, automatic_simplify_product },
    SimplificationRule{ match_type<Power>, automatic_simplify_power },
    SimplificationRule{ match_type<Power>, automatic_simplify_power },

};


struct Simplifier{
    ExprPtr automatic_simplify(ExprPtr x){
        SimplificationContext sc;
        for(const auto&[match, transform] : simplification_rules){
            if(match(x)){
                x = transform(sc, std::move(x));
            }
        }
        return x;
    }
};