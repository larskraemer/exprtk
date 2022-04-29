#pragma once
#include <algorithm>
#include <vector>
#include <string>
#include <ranges>

#include <fmt/format.h>

#include "math/math_functions.hpp"
#include "expression.hpp"
#include "simplify.hpp"

namespace symb{


class Symbolic{
public:
    friend class fmt::formatter<Symbolic>;
    friend class math::impl::pow<Symbolic,Symbolic>;

    Symbolic(impl::ExprPtr e)
        : m_expr{impl::Simplifier{}.automatic_simplify(std::move(e))}
    {}

    Symbolic(const Symbolic& other)
        : m_expr{other.m_expr->copy()}
    {}

    auto& operator=(const Symbolic& other){
        m_expr = other.m_expr->copy();
        return *this;
    }

    Symbolic(Symbolic&& other)
        : m_expr{std::move(other.m_expr)}
    {}

    auto& operator=(Symbolic&& other){
        m_expr = std::move(other.m_expr);
        return *this;
    }

    friend Symbolic operator+(Symbolic lhs, Symbolic rhs) {
        return Symbolic{
            impl::make_expression<impl::Sum>(
                std::move(lhs.m_expr), 
                std::move(rhs.m_expr)
            )
        };
    }

    friend Symbolic operator-(Symbolic op){
        return Symbolic{
            impl::make_expression<impl::Product>(
                impl::make_expression<impl::Number>(-1),
                std::move(op.m_expr)
            )
        };
    }

    friend Symbolic operator-(Symbolic lhs, const Symbolic& rhs) {
        return lhs + (-rhs);
    }

    friend Symbolic operator*(Symbolic lhs, Symbolic rhs) {
        return Symbolic{
            impl::make_expression<impl::Product>(
                std::move(lhs.m_expr), 
                std::move(rhs.m_expr)
            )
        };
    }

    friend Symbolic operator/(Symbolic lhs, Symbolic rhs) {
        return Symbolic{ 
            impl::make_expression<impl::Product>(
                std::move(lhs.m_expr),
                impl::make_expression<impl::Power>(
                    std::move(rhs.m_expr),
                    impl::make_expression<impl::Number>(-1)
                )
            )
        };
    }

    friend auto func(std::string name);
private:
    impl::ExprPtr m_expr;
};



template<class T> requires std::constructible_from<impl::ExpressionBase::Number_t, T>
Symbolic num(T v) {
    return Symbolic(std::make_unique<impl::Number>(v));
}

Symbolic var(std::string name) {
    return Symbolic(impl::make_expression<impl::Symbol>(name));
}

auto func(std::string name){
    return [name]<class... Ts>(Ts&&... ts){
        std::vector<impl::ExprPtr> exprs;
        (
            [&]<class T>(T&& x) {
                if constexpr (std::is_same_v<std::remove_cvref_t<T>, Symbolic>){
                    exprs.emplace_back(std::forward<T>(x).m_expr->copy());
                }
                else if constexpr(std::is_constructible_v<impl::ExpressionBase::Number_t, T>){
                    exprs.emplace_back(impl::make_expression<impl::Number>(std::forward<T>(x)));
                }
            }(std::forward<Ts>(ts)), ...
        );
        return Symbolic(
            impl::make_expression<impl::Function>(
                name,
                std::move(exprs)
            )
        );
    };
}

} // namespace symb


template<>
struct fmt::formatter<symb::Symbolic> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin();
        auto end = ctx.end();
        if(it != end && *it != '}') throw format_error("invalid format");
        return it;
    }

    template<typename FormatContext>
    auto format(const symb::Symbolic& s, FormatContext& ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), "{}", s.m_expr->str());
    }
};

template<>
struct math::impl::pow<symb::Symbolic, symb::Symbolic>{
    static auto func(symb::Symbolic b, symb::Symbolic e) -> symb::Symbolic {
        return symb::impl::Simplifier{}.automatic_simplify(
            symb::impl::make_expression<symb::impl::Power>(
                std::move(b.m_expr), 
                std::move(e.m_expr)
            )
        );
    }
};

template<class T>
struct math::impl::pow<symb::Symbolic, T>{
    static auto func(symb::Symbolic b, T e) -> symb::Symbolic {
        return math::pow(b, symb::num(e));
    }
};