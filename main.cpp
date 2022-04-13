#include "symbolic.hpp"
#include <iostream>

#include <fmt/format.h>



int main() {
    auto x = Symbolic::var("x");
    auto y = Symbolic::var("y");
    auto z = Symbolic::var("z");

    auto a = (x+y);
    a = a * z;

    fmt::print("{}\n", a);

    auto b = math::pow(math::pow(a, Symbolic::num(2)), Symbolic::num(1)) / Symbolic::num(2);

    fmt::print("{}\n", b);
}