#pragma once

#include <algorithm>
#include <string_view>
#include <compare>
#include <string>
#include <span>

#include "gmp.h"
#include "math_functions.hpp"
#include <fmt/format.h>


namespace multiprecision{

class MPi final {
public:
    [[nodiscard]] MPi(){ 
        mpz_init(m_handle); 
    }

    template<std::signed_integral T>
    [[nodiscard]] explicit MPi(T value) {
        mpz_init_set_si(m_handle, static_cast<long>(value));
    }

    template<std::unsigned_integral T>
    [[nodiscard]] explicit MPi(T value) {
        mpz_init_set_ui(m_handle, static_cast<unsigned long>(value));
    }

    [[nodiscard]] explicit MPi(std::string_view sv){
        mpz_init_set_str(m_handle, sv.data(), static_cast<int>(sv.size()));
    }

    MPi(const MPi& other){
        mpz_init_set(m_handle, other.m_handle);
    }

    MPi& operator=(const MPi& other){
        mpz_set(m_handle, other.m_handle);
        return *this;
    }

    MPi(MPi&& other)
        : MPi{}
    {
        mpz_swap(m_handle, other.m_handle);
    }

    MPi& operator=(MPi&& other) {
        mpz_swap(m_handle, other.m_handle);
        return *this;
    }

    ~MPi(){
        mpz_clear(m_handle);
    }

    /* ***********************************************
        Output
    ************************************************** */

    friend std::string to_string(const MPi& value) {
        std::string ret;
        ret.resize(mpz_sizeinbase(value.m_handle, 10)+1);
        mpz_get_str(ret.data(), 10, value.m_handle);
        return ret;
    }

    /* ***********************************************
        Comparision Operators
    ************************************************** */

    friend auto operator<=>(const MPi& lhs, const MPi& rhs) -> std::strong_ordering {
        auto c = mpz_cmp(lhs.m_handle, rhs.m_handle);
        if(c > 0) return std::strong_ordering::greater;
        else if(c == 0) return std::strong_ordering::equal;
        else return std::strong_ordering::less;
    }

    template<std::integral T>
    friend auto operator<=>(const MPi& lhs, T rhs) -> std::strong_ordering {
        int c = 0;
        if constexpr(std::signed_integral<T>) c = mpz_cmp_si(lhs.m_handle, static_cast<long>(rhs));
        else c = mpz_cmp_ui(lhs.m_handle, static_cast<unsigned long>(rhs));
        if(c > 0) return std::strong_ordering::greater;
        else if(c == 0) return std::strong_ordering::equal;
        else return std::strong_ordering::less;
    }

    template<std::integral T>
    friend auto operator<=>(T lhs, const MPi& rhs) -> std::strong_ordering {
        auto c = rhs <=> lhs;
        if(c == std::strong_ordering::less) return std::strong_ordering::greater;
        if(c == std::strong_ordering::equal) return std::strong_ordering::equal;
        if(c == std::strong_ordering::greater) return std::strong_ordering::less;
    }

    template<std::integral T>
    friend auto operator==(const MPi& lhs, T rhs) -> bool {
        return (lhs <=> rhs) == 0;
    }

    template<std::integral T>
    friend auto operator==(T lhs, const MPi& rhs) -> bool {
        return (lhs <=> rhs) == 0;
    }


    /* ***********************************************
        Arithmetic Operators
    ************************************************** */

    //Negation
    auto operator-() const -> MPi {
        MPi ret;
        mpz_neg(ret.m_handle, m_handle);
        return ret;
    }

    //Addition
    auto operator+=(const MPi& other) -> MPi& {
        mpz_add(m_handle, m_handle, other.m_handle);
        return *this;
    }

    template<std::integral T>
    auto operator+=(T value) -> MPi& {
        if constexpr(std::unsigned_integral<T>) mpz_add_ui(m_handle, m_handle, static_cast<unsigned long>(value));
        else {
            if(value < 0) mpz_sub_ui(m_handle, m_handle, static_cast<unsigned long>(-value));
            else mpz_add_ui(m_handle, m_handle, static_cast<unsigned long>(value));
        }
        return *this;
    }

    friend auto operator+(MPi lhs, const MPi& rhs) -> MPi {
        lhs += rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator+(MPi lhs, T rhs) -> MPi {
        lhs += rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator+(T lhs, MPi rhs) -> MPi {
        rhs += lhs;
        return rhs;
    }

    //Subtraction
    auto operator-=(const MPi& other) -> MPi& {
        mpz_sub(m_handle, m_handle, other.m_handle);
        return *this;
    }

    template<std::integral T>
    auto operator-=(T value) -> MPi& {
        if constexpr(std::unsigned_integral<T>) mpz_sub_ui(m_handle, m_handle, static_cast<unsigned long>(value));
        else {
            if(value < 0) mpz_add_ui(m_handle, m_handle, static_cast<unsigned long>(-value));
            else mpz_sub_ui(m_handle, m_handle, static_cast<unsigned long>(value));
        }
        return *this;
    }

    friend auto operator-(MPi lhs, const MPi& rhs) -> MPi {
        lhs -= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator-(MPi lhs, T rhs) -> MPi {
        lhs -= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator-(T lhs, MPi rhs) -> MPi {
        rhs -= lhs;
        return rhs;
    }

    //Multiplication
    auto operator*=(const MPi& other) -> MPi& {
        mpz_mul(m_handle, m_handle, other.m_handle);
        return *this;
    }

    template<std::integral T>
    auto operator*=(T value) -> MPi& {
        if constexpr(std::unsigned_integral<T>) mpz_mul_ui(m_handle, m_handle, static_cast<unsigned long>(value));
        else mpz_mul_si(m_handle, m_handle, static_cast<long>(value));
        return *this;
    }

    friend auto operator*(MPi lhs, const MPi& rhs) -> MPi {
        lhs *= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator*(MPi lhs, T rhs) -> MPi {
        lhs *= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator*(T lhs, MPi rhs) -> MPi {
        rhs *= lhs;
        return rhs;
    }

    //Division
    auto operator/=(const MPi& other) -> MPi& {
        mpz_tdiv_q(m_handle, m_handle, other.m_handle);
        return *this;
    }

    template<std::integral T>
    auto operator/=(T value) -> MPi& {
        return (*this) /= MPi(value);
    }

    friend auto operator/(MPi lhs, const MPi& rhs) -> MPi {
        lhs /= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator/(MPi lhs, T rhs) -> MPi {
        return lhs / MPi(rhs);
    }

    template<std::integral T>
    friend auto operator/(T lhs, MPi rhs) -> MPi {
        return MPi(lhs) / rhs;
    }

    //Modulo
    auto operator%=(const MPi& other) -> MPi& {
        mpz_tdiv_r(m_handle, m_handle, other.m_handle);
        return *this;
    }

    template<std::integral T>
    auto operator%=(T value) -> MPi& {
        return (*this) %= MPi(value);
    }

    friend auto operator%(MPi lhs, const MPi& rhs) -> MPi {
        lhs %= rhs;
        return lhs;
    }

    template<std::integral T>
    friend auto operator%(MPi lhs, T rhs) -> MPi {
        if constexpr(std::unsigned_integral<T>) {
            mpz_tdiv_r_ui(lhs.m_handle, lhs.m_handle, static_cast<unsigned long>(rhs));
            return lhs;
        }
        else{
            if(rhs < 0) return -(lhs % static_cast<unsigned long>(-rhs));
            else return lhs % static_cast<unsigned long>(rhs);
        }
    }

    template<std::integral T>
    friend auto operator%(T lhs, MPi rhs) -> MPi {
        return MPi(lhs) % rhs;
    }

    /* ***********************************************
        Misc
    ************************************************** */

    auto handle() const -> const mpz_t& { return m_handle; }
    auto handle() -> mpz_t& { return m_handle; }
private:
    mpz_t m_handle = {};
};
}

template<>
struct math::impl::is_integer<multiprecision::MPi>{
    static auto func(const multiprecision::MPi&) -> bool { return true; }
};

template<>
struct math::impl::sign<multiprecision::MPi>{
    static auto func(const multiprecision::MPi& x) {
        return mpz_sgn(x.handle());
    }
};

template<>
struct math::impl::gcd<multiprecision::MPi, multiprecision::MPi> {
    static auto func(const multiprecision::MPi& a, const multiprecision::MPi& b) -> multiprecision::MPi {
        //fmt::print("GCD input: {}, {}\n", introspect_mpz_t(a.handle()), introspect_mpz_t(b.handle()));
        multiprecision::MPi ret;
        mpz_gcd(ret.handle(), a.handle(), b.handle());
        //fmt::print("GCD result: {}\n", introspect_mpz_t(ret.handle()));
        return ret;
    }
};

template<>
struct math::impl::pow<multiprecision::MPi, multiprecision::MPi>{
    static auto func(multiprecision::MPi base, multiprecision::MPi exp) -> multiprecision::MPi {
        if(exp < 0) {
            return multiprecision::MPi(0);
        }
        auto ret = multiprecision::MPi(1);
        while(exp > 0){
            if(mpz_divisible_2exp_p(exp.handle(), 1)){
                mpz_divexact_ui(exp.handle(), exp.handle(), 2);   // Divide exponent by 2
                mpz_mul(base.handle(), base.handle(), base.handle());   // square base
            }
            else{
                ret *= base;
                mpz_tdiv_q_2exp(exp.handle(), exp.handle(), 1); // floor-divide exponent by 2
                mpz_mul(base.handle(), base.handle(), base.handle());   // square base
            }
        }
        return ret;
    }
};