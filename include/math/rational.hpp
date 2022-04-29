#pragma once

#include <concepts>
#include <string>
#include <fmt/format.h>

#include "math_functions.hpp"


template<class T>
class FieldOfFractions {
public:
    constexpr FieldOfFractions(T num = T{0}, T denom = T{1}, bool is_coprime = false)
        : m_num{std::move(num)}, m_denom{std::move(denom)}
    {
        if(not is_coprime) simplify_fraction();
    }

    template<class U> requires std::constructible_from<T, U>
    constexpr explicit FieldOfFractions(U num, U denom = U{1})
        : m_num{std::move(num)}, m_denom{std::move(denom)}
    {
        simplify_fraction();
    }

    constexpr auto num() const { return m_num; }
    constexpr auto denom() const { return m_denom; }

    constexpr auto& operator+=(const FieldOfFractions& other) {
        m_num = m_num * other.m_denom + other.m_num * m_denom;
        m_denom *= other.m_denom;
        simplify_fraction();
        return *this;
    }

    constexpr auto& operator-=(const FieldOfFractions& other) {
        m_num = m_num * other.m_denom - other.m_num * m_denom;
        m_denom *= other.m_denom;
        simplify_fraction();
        return *this;
    }

    constexpr auto& operator*=(const FieldOfFractions& other) {
        m_num *= other.m_num;
        m_denom *= other.m_denom;
        simplify_fraction();
        return *this;
    }

    constexpr auto& operator/=(const FieldOfFractions& other) {
        m_num *= other.m_denom;
        m_denom *= other.m_num;
        simplify_fraction();
        return *this;
    }

    friend constexpr auto operator+(FieldOfFractions lhs, const FieldOfFractions& rhs){
        lhs += rhs;
        return lhs;
    }

    friend constexpr auto operator-(FieldOfFractions lhs, const FieldOfFractions& rhs){
        lhs -= rhs;
        return lhs;
    }

    friend constexpr auto operator*(FieldOfFractions lhs, const FieldOfFractions& rhs){
        lhs *= rhs;
        return lhs;
    }

    friend constexpr auto operator/(FieldOfFractions lhs, const FieldOfFractions& rhs){
        lhs /= rhs;
        return lhs;
    }

    friend constexpr std::strong_ordering operator<=>(const FieldOfFractions& lhs, const FieldOfFractions& rhs){
        return lhs.num() * rhs.denom() <=> rhs.num() * lhs.denom();
    }

    template<class U>
    friend constexpr std::strong_ordering operator<=>(const FieldOfFractions& lhs, const U& rhs) {
        return lhs.num() <=> rhs * lhs.denom();
    }

    template<class U>
    friend constexpr std::strong_ordering operator<=>(const U& lhs, const FieldOfFractions& rhs) {
        return lhs * rhs.denom() <=> rhs.num();
    }

    friend constexpr bool operator==(const FieldOfFractions& lhs, const FieldOfFractions& rhs) {
        return (lhs <=> rhs) == 0;
    }

    template<class U>
    friend constexpr bool operator==(const FieldOfFractions& lhs, const U& rhs) {
        return (lhs <=> rhs) == 0;
    }

    template<class U>
    friend constexpr bool operator==(const U& lhs, const FieldOfFractions& rhs) {
        return (lhs <=> rhs) == 0;
    }

private:
    void simplify_fraction(){
        // Simplify the fraction
        auto g = math::gcd(m_num, m_denom);
        m_num /= g;
        m_denom /= g;
        // The sign must be in the numerator
        m_num *= math::sign(m_denom);
        m_denom *= math::sign(m_denom);
    }

    T m_num;
    T m_denom;
};

template<class T>
std::string to_string(const FieldOfFractions<T>& f) {
    using std::to_string;
    if(f.denom() == 1){
        return to_string(f.num());
    }
    else{
        return to_string(f.num()) + "/" + to_string(f.denom());
    }
}

template<class T, std::integral U>
struct math::impl::pow<FieldOfFractions<T>, U>{
    static constexpr auto func(FieldOfFractions<T> base, U exponent) -> FieldOfFractions<T>{
        if(exponent < 0) return math::impl::pow<FieldOfFractions<T>, U>::func(1/base, -exponent);
        auto ret = FieldOfFractions<T>{1};
        while(exponent != 0){
            if(exponent % 2 == 0){
                base *= base;
                exponent /= 2;
            }
            else{
                ret *= base;
                exponent -= 1;
            }
        }
        return ret;
    }
};

template<class T>
struct math::impl::is_integer<FieldOfFractions<T>>{
    static constexpr auto func(const FieldOfFractions<T>& f){
        return (f.denom() == 1) && math::is_integer(f.num());
    }
};

template<class T>
struct math::impl::pow<FieldOfFractions<T>, FieldOfFractions<T>>{
    static auto func(const FieldOfFractions<T>& base, const FieldOfFractions<T>& exponent) {
        if(math::is_integer(exponent)){
            return math::pow(base, exponent.num());
        }
        else{
            // Try to find an exact root of base, maybe?
            throw std::runtime_error("Invalid arguments to pow.");
        }    
    }
};

template<class T>
struct math::impl::pow<FieldOfFractions<T>, T>{
    static auto func(const FieldOfFractions<T>& base, const T& exponent) {
        if(exponent < 0) return FieldOfFractions<T>(
            math::pow(base.denom(), -exponent),
            math::pow(base.num(), -exponent)
        );
        else return FieldOfFractions<T>(
            math::pow(base.num(), exponent),
            math::pow(base.denom(), exponent)
        );
    }
};

template<class T>
struct fmt::formatter<FieldOfFractions<T>>{
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin();
        auto end = ctx.end();
        if(it != end && *it != '}') throw format_error("invalid format");
        return it;
    }

    template<typename FormatContext>
    auto format(const FieldOfFractions<T>& s, FormatContext& ctx) -> decltype(ctx.out()) {
        if(s.denom() == 1){
            return format_to(ctx.out(), "{}", s.num());
        }
        else{
            return format_to(ctx.out(), "{}/{}", s.num(), s.denom());
        }
    }
};