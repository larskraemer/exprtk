#include "symbolic/symbolic.hpp"
#include <iostream>

#include <fmt/format.h>

int main() {
    auto x = symb::var("x");
    auto f = symb::func("f");

    auto a = math::pow(f(x), 101);

    fmt::print("{}\n", a);

    auto b = symb::func("diff")(a, x); 

    fmt::print("{}\n", b);
}