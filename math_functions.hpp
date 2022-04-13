#pragma once

#include <type_traits>
#include <concepts>

namespace math {

namespace impl{
template<class T, class U>
struct pow{};

// Overload for integer exponent
template<class T, std::integral U>
struct pow<T, U>{
    constexpr static auto func(T t, U u) -> T{
        if(u < 0){
            return func(1/t, -u);
        }

        auto ret = T{1};
        while(u != 0){
            if(u % 2 == 0){
                t *= t;
                u /= 2;
            }
            else{
                ret *= t;
                u -= 1;
            }
        }
        return ret;
    }
};

template<class T, class U>
struct gcd{};

template<std::integral T, std::integral U>
struct gcd<T, U>{
    constexpr static auto func(T a, T b) -> std::common_type_t<T, U> {
        using V = std::common_type_t<T, U>;
        return gcd<V, V>::func(static_cast<V>(a), static_cast<V>(b));
    }
};

template<std::integral T>
struct gcd<T, T>{
    constexpr static auto func(T a, T b) -> T {
        while(b != 0){
            auto tmp = static_cast<T>(a % b);
            a = b;
            b = tmp;
        }
        return a;
    }
};

template<class T>
struct sign{};

template<std::integral T>
struct sign<T>{
    static constexpr auto func(T x) -> T {
        if constexpr(std::unsigned_integral<T>) return 1;
        else{
            if(x < 0) return -1;
            else if(x == 0) return 0;
            else return 1;
        }
    }
};

template<class T>
struct is_integer{};

template<std::integral T>
struct is_integer<T>{
    constexpr static bool func(const T&) { return true; }
};
}

template<class T, class U>
constexpr auto pow(T&& t, U&& u) {
    return impl::pow<std::remove_cvref_t<T>, std::remove_cvref_t<U>>::func(std::forward<T>(t), std::forward<U>(u));
}

template<class T, class U>
constexpr auto gcd(T&& a, U&& b) {
    return impl::gcd<std::remove_cvref_t<T>, std::remove_cvref_t<U>>::func(std::forward<T>(a), std::forward<U>(b));
}

template<class T>
constexpr auto sign(T&& a) {
    return impl::sign<std::remove_cvref_t<T>>::func(std::forward<T>(a));
}

template<class T>
constexpr auto is_integer(T&& a) -> bool {
    return impl::is_integer<std::remove_cvref_t<T>>::func(std::forward<T>(a));
}

}