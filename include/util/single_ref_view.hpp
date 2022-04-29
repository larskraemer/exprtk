#pragma once

#include <ranges>
#include <optional>

template<class T>
struct single_cref_view {
    single_cref_view(const T& value)
        : m_value{value}
    {}

    auto data() const { return &m_value; }
    auto begin() const { return data(); }
    auto end() const { return data() + 1; }
    auto size() const -> size_t { return 1; }
    auto operator[](size_t i) const -> auto& { return *(begin() + i); }

    const T &m_value;
};