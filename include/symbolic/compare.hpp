#pragma once

#include <compare>
#include <ranges>


#include "expression.hpp"
#include "util/single_ref_view.hpp"

namespace symb{
namespace impl{

template<std::ranges::range A, std::ranges::range B>
[[nodiscard]] std::strong_ordering cmp_expression_list(A lhs, B rhs);

[[nodiscard]] std::strong_ordering cmp_expression(const ExprPtr& lhs, const ExprPtr& rhs);

[[nodiscard]] constexpr auto cmp_kind(Kind a, Kind b) -> std::strong_ordering {
    return static_cast<int>(a) <=> static_cast<int>(b);
}

template<std::ranges::range A, std::ranges::range B>
[[nodiscard]] std::strong_ordering cmp_expression_list(A lhs, B rhs){
    auto N = std::min(lhs.size(), rhs.size());
    for(auto k = 0u; k < N; k++){
        auto v = cmp_expression(lhs[lhs.size() - k - 1], rhs[rhs.size() - k - 1]);
        if(v != 0) return v;
    }
    return lhs.size() <=> rhs.size();
}

[[nodiscard]] inline std::strong_ordering cmp_base(const ExprPtr& lhs, const ExprPtr& rhs) {
    if(lhs->kind() == Kind::PowOp && rhs->kind() == Kind::PowOp) {
        return cmp_expression(lhs->children[0], rhs->children[0]);
    }
    else if(lhs->kind() == Kind::PowOp){
        return cmp_expression(lhs->children[0], rhs);
    }
    else if(rhs->kind() == Kind::PowOp){
        return cmp_expression(lhs, rhs->children[0]);
    }
    else{
        return cmp_expression(lhs, rhs);
    }
}
} //namespace impl
} //namespace symb