#pragma once
#include <algorithm>
#include <vector>
#include <variant>
#include <compare>
#include <string>
#include <span>
#include <ranges>
#include <limits>

#include <fmt/format.h>

#include "math_functions.hpp"
#include "expression.hpp"

class Symbolic{
public:
    friend class fmt::formatter<Symbolic>;
    friend class math::impl::pow<Symbolic,Symbolic>;

    static Symbolic num(ExpressionBase::Number_t v) {
        return Symbolic(std::make_unique<Number>(v));
    }

    static Symbolic var(std::string name) {
        return Symbolic(std::make_unique<Symbol>(name));
    }

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
            Simplifier{}.automatic_simplify(
                std::make_unique<Sum>(
                    std::move(lhs.m_expr), 
                    std::move(rhs.m_expr)
                )
            )
        };
    }

    friend Symbolic operator*(Symbolic lhs, Symbolic rhs) {
        return Symbolic{
            Simplifier{}.automatic_simplify(
                std::make_unique<Product>(
                    std::move(lhs.m_expr), 
                    std::move(rhs.m_expr)
                )
            )
        };
    }

    friend Symbolic operator/(Symbolic lhs, Symbolic rhs) {
        return Symbolic{ 
            Simplifier{}.automatic_simplify(
                std::make_unique<Product>(
                    std::move(lhs.m_expr),
                    std::make_unique<Power>(
                        std::move(rhs.m_expr),
                        std::make_unique<Number>(-1)
                    )
                )
            )
        };
    }
private:
    Symbolic(ExprPtr e)
        : m_expr{std::move(e)}
    {}

    ExprPtr m_expr;
};



template<>
struct fmt::formatter<Symbolic> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin();
        auto end = ctx.end();
        if(it != end && *it != '}') throw format_error("invalid format");
        return it;
    }

    template<typename FormatContext>
    auto format(const Symbolic& s, FormatContext& ctx) -> decltype(ctx.out()) {
        return format_to(ctx.out(), "{}", expr_to_str(s.m_expr));
    }
};

template<>
struct math::impl::pow<Symbolic, Symbolic>{
    static auto func(Symbolic b, Symbolic e) -> Symbolic {
        return Simplifier{}.automatic_simplify(
            std::make_unique<Power>(
                std::move(b.m_expr), 
                std::move(e.m_expr)
            )
        );
    }
};