#pragma once

#include "concepts.hpp"
#include <compare>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace mystl {

namespace detail {

// ****************************************************************************
// *                          unique_ptr helper                               *
// ****************************************************************************

template <class From, class To>
concept pointer_convertible_to = std::convertible_to<From*, To*>;

template <class From, class To>
concept deleter_copy_constructible_to =
    std::is_reference_v<From> && std::is_nothrow_constructible_v<To, From>;

template <class From, class To>
concept deleter_move_constructible_to =
    (!std::is_reference_v<From>) &&
    std::is_nothrow_constructible_v<To, From &&>;

template <class From, class To>
concept deleter_copy_assignable_to =
    std::is_reference_v<From> && std::is_nothrow_assignable_v<To, From>;

template <class From, class To>
concept deleter_move_assignable_to =
    (!std::is_reference_v<From>) && std::is_nothrow_assignable_v<To, From &&>;
} // namespace detail

template <class T> struct default_delete {
    constexpr default_delete() noexcept = default;

    template <detail::pointer_convertible_to<T> U>
    constexpr default_delete(const default_delete<U> &) noexcept {}

    constexpr void operator()(T *ptr) const {
        static_assert(complete<T>);
        ::delete ptr;
    }
};

template <class T> struct default_delete<T[]> {
    constexpr default_delete() noexcept = default;

    template <detail::pointer_convertible_to<T> U>
    constexpr default_delete(const default_delete<U[]> &) noexcept {}

    constexpr void operator()(T *ptr) const {
        static_assert(complete<T>);
        ::delete[] ptr;
    }
};

// ****************************************************************************
// *                                 unique_ptr                               *
// ****************************************************************************

template <class T, class Deleter = default_delete<T>>
    requires(!std::is_rvalue_reference_v<Deleter>) &&
            std::invocable<Deleter, T *>
class unique_ptr {
  public:
    using pointer = T *;
    using element_type = T;
    using deleter_type = Deleter;

    // constructors
    //  regular constructors
    constexpr unique_ptr() noexcept
        requires std::is_default_constructible_v<Deleter>
        : _ptr{nullptr}, _deleter{} {}
    constexpr unique_ptr(std::nullptr_t) noexcept
        requires std::is_default_constructible_v<Deleter>
        : _ptr{nullptr}, _deleter{} {}
    constexpr explicit unique_ptr(pointer p) noexcept
        requires std::is_default_constructible_v<Deleter>
        : _ptr{p}, _deleter{} {}

    constexpr unique_ptr(pointer p,
                         std::conditional_t<std::is_reference_v<Deleter> &&
                                                (!std::is_const_v<Deleter>),
                                            Deleter, const Deleter &>
                             d) noexcept
        requires std::is_nothrow_copy_constructible_v<Deleter>
        : _ptr{p}, _deleter{d} {}

    constexpr unique_ptr(pointer p, Deleter &&d) noexcept
        requires(!std::is_reference_v<Deleter>) &&
                    std::is_nothrow_move_constructible_v<Deleter>
        : _ptr{p}, _deleter{std::move(d)} {}

    // copy/move constructors
    template <detail::pointer_convertible_to<T> U,
              detail::deleter_copy_constructible_to<Deleter> OtherDeleter>
    constexpr unique_ptr(unique_ptr<U, OtherDeleter> &&other) noexcept
        : _ptr(other.release()), _deleter(other.get_deleter()) {}

    template <detail::pointer_convertible_to<T> U,
              detail::deleter_move_constructible_to<Deleter> OtherDeleter>
    constexpr unique_ptr(unique_ptr<U, OtherDeleter> &&other) noexcept
        : _ptr(other.release()), _deleter(std::move(other.get_deleter())) {}

    // destructor
    constexpr ~unique_ptr() { _deleter(_ptr); }

    // assignments
    constexpr auto operator=(std::nullptr_t) -> unique_ptr & {
        reset();
        return *this;
    }

    template <detail::pointer_convertible_to<T> U,
              detail::deleter_copy_assignable_to<Deleter> OtherDeleter>
    constexpr auto operator=(unique_ptr<U, OtherDeleter> &&other) noexcept
        -> unique_ptr & {
        if (static_cast<void*>(this) == static_cast<void*>(&other))
            return *this;   // self-move-assignment is no-op
        reset();
        _ptr = other.release();
        _deleter = other.get_deleter();
        return *this;
    }

    template <detail::pointer_convertible_to<T> U,
              detail::deleter_move_assignable_to<Deleter> OtherDeleter>
    constexpr auto operator=(unique_ptr<U, OtherDeleter> &&other) noexcept
        -> unique_ptr & {
        if (static_cast<void*>(this) == static_cast<void*>(&other))
            return *this;   // self-move-assignment is no-op
        reset();
        _ptr = other.release();
        _deleter = std::move(other.get_deleter());
        return *this;
    }

    // modifiers
    constexpr auto release() noexcept -> pointer {
        return std::exchange(_ptr, nullptr);
    }
    constexpr auto reset(pointer ptr = nullptr) noexcept -> void {
        if (ptr == _ptr)
            return; // no-op if ptr the same as _ptr
        _deleter(std::exchange(_ptr, ptr));
    }
    constexpr auto swap(unique_ptr &other) noexcept -> void {
        _ptr = std::exchange(other._ptr, _ptr);
        _deleter = std::exchange(other._deleter, _deleter);
    }

    // observers
    constexpr auto get() const noexcept -> pointer { return _ptr; }
    constexpr auto get_deleter() noexcept -> deleter_type & { return _deleter; }
    constexpr auto get_deleter() const noexcept -> const deleter_type & {
        return _deleter;
    }
    constexpr explicit operator bool() const noexcept {
        return get() != nullptr;
    }

    constexpr auto operator*() const
        noexcept(noexcept(*std::declval<pointer>())) -> T & {
        return *get();
    }
    constexpr auto operator->() const noexcept -> pointer { return get(); }

    // compare
    template <class U, class D>
    [[nodiscard]] constexpr auto
    operator==(const unique_ptr<U, D> &rhs) const noexcept -> bool {
        return get() == rhs.get();
    }

    [[nodiscard]] constexpr auto operator==(std::nullptr_t) const noexcept
        -> bool {
        return get() == nullptr;
    }

    template <class T2, class D2>
        requires std::three_way_comparable_with<
                     pointer, typename unique_ptr<T2, D2>::pointer>
    [[nodiscard]] constexpr auto
    operator<=>(const unique_ptr<T2, D2> &rhs) const noexcept
        -> std::compare_three_way_result_t<
            pointer, typename unique_ptr<T2, D2>::pointer> {
        return get() <=> rhs.get();
    }

  private:
    pointer _ptr;
    [[no_unique_address]] deleter_type _deleter;
};

template <class T, class... Args>
constexpr auto make_unique(Args &&...args) -> unique_ptr<T> {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

} // namespace mystl