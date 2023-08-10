#pragma once

#include "concepts.hpp"
#include <concepts>
#include <type_traits>

namespace mystl {

// ****************************************************************************
// *                                 unique_ptr                               *
// ****************************************************************************

template <class T> struct default_delete {
    constexpr default_delete() noexcept = default;

    template <class U>
        requires std::convertible_to<U, T>
    constexpr default_delete(const default_delete<U> &d) noexcept {}

    constexpr void operator()(T *ptr) const {
        static_assert(complete<T>);
        delete ptr;
    }
};

template <class T> struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;

    template <class U>
        requires std::convertible_to<U, T>
    constexpr default_delete(const default_delete<U[]> &d) noexcept {}

    constexpr void operator()(T *ptr) const {
        static_assert(complete<T>);
        delete[] ptr;
    }
};

} // namespace mystl