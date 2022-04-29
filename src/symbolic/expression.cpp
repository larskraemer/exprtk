#include "symbolic/expression.hpp"

namespace symb{
namespace impl{
auto ExpressionBase::constant() const -> ExprPtr {
    return make_expression<Number>(1);
}

auto ExpressionBase::term() const -> ExprPtr {
    return copy();
}

auto ExpressionBase::base() const -> ExprPtr {
    return copy();
}

auto ExpressionBase::exponent() const -> ExprPtr {
    return make_expression<Number>(1);
}

} // namespace impl
} // namespace symb
