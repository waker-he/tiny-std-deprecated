#pragma once

#include <array>
#include <concepts>
#include <initializer_list>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include "memory.hpp"

namespace mystd {

class any;

namespace detail {

struct AnyBase {
    virtual ~AnyBase() = default;
    virtual auto clone() const -> unique_ptr<AnyBase> = 0;
    virtual auto type() const noexcept -> const std::type_info & = 0;
};

template <class T> struct AnyDerive : AnyBase {
    T value_;

    explicit AnyDerive(const T &t) : value_(t) {}
    explicit AnyDerive(T &&t) : value_(std::move(t)) {}

    template <class... Args>
    explicit AnyDerive(Args &&...args) : value_(std::forward<Args>(args)...) {}

    auto clone() const -> unique_ptr<AnyBase> override {
        return make_unique<AnyDerive<T>>(value_);
    }

    auto type() const noexcept -> const std::type_info & override {
        return typeid(T);
    }
};

template <class T>
concept is_any = std::same_as<std::decay_t<T>, any>;

template <class T>
concept any_constructible =
    (!is_any<T>) && std::copy_constructible<std::decay_t<T>>;

template <class T, class... Args>
concept any_constructible_from =
    any_constructible<T> && std::constructible_from<std::decay_t<T>, Args...>;
} // namespace detail

class bad_any_cast : public std::bad_cast {
  public:
    virtual auto what() const noexcept -> const char * {
        return "bad any_cast";
    }
};

class any {
  public:
    // constructors
    constexpr any() noexcept : ptr_(nullptr) {}

    template <class T>
        requires detail::any_constructible<T>
    any(T &&value)
        : ptr_{make_unique<detail::AnyDerive<std::decay_t<T>>>(
              std::forward<T>(value))} {}

    any(const any &other) : ptr_(other.ptr_ ? other.ptr_->clone() : nullptr) {}
    any(any &&) noexcept = default;

    template <class T, class... Args>
        requires detail::any_constructible_from<T, Args...>
    explicit any(std::in_place_type_t<T>, Args &&...args) {
        emplace<T>(std::forward<Args>(args)...);
    }

    template <class T, class U, class... Args>
        requires detail::any_constructible_from<T, std::initializer_list<U>,
                                                Args...>
    explicit any(std::in_place_type_t<T>, std::initializer_list<U> il,
                 Args &&...args) {
        emplace<T>(il, std::forward<Args>(args)...);
    }

    // destructor
    ~any() = default;

    // assignment operators
    auto operator=(const any &rhs) -> any & {
        if (this == &rhs)
            return *this;
        ptr_ = rhs.ptr_ ? rhs.ptr_->clone() : nullptr;
        return *this;
    }
    auto operator=(any &&) noexcept -> any & = default;

    template <class T> auto operator=(T &&value) -> any & {
        return *this = any(std::forward<T>(value));
    }

    // modifiers
    template <class T, class... Args>
        requires detail::any_constructible_from<T, Args...>
    auto emplace(Args &&...args) -> std::decay_t<T> & {
        ptr_ = make_unique<detail::AnyDerive<std::decay_t<T>>>(
            std::forward<Args>(args)...);
        return static_cast<detail::AnyDerive<std::decay_t<T>> *>(ptr_.get())
            ->value_;
    }

    template <class T, class U, class... Args>
        requires detail::any_constructible_from<T, std::initializer_list<U>,
                                                Args...>
    auto emplace(std::initializer_list<U> il, Args &&...args)
        -> std::decay_t<T> & {
        ptr_ = make_unique<detail::AnyDerive<std::decay_t<T>>>(
            il, std::forward<Args>(args)...);
        return static_cast<detail::AnyDerive<std::decay_t<T>> *>(ptr_.get())
            ->value_;
    }

    auto reset() noexcept -> void { ptr_.reset(); }

    auto swap(any &other) noexcept -> void { ptr_.swap(other.ptr_); }

    // observers
    auto has_value() const noexcept -> bool { return static_cast<bool>(ptr_); }

    auto type() const noexcept -> const std::type_info & {
        return has_value() ? ptr_->type() : typeid(void);
    }

  private:
    unique_ptr<detail::AnyBase> ptr_;

    template <class T>
    friend auto any_cast(const any *a) noexcept -> const T * {
        if (a == nullptr)
            return nullptr;
        if (a->type() == typeid(T))
            return &static_cast<detail::AnyDerive<std::remove_const_t<T>> *>(
                        a->ptr_.get())
                        ->value_;
        return nullptr;
    }

    template <class T> friend auto any_cast(any *a) noexcept -> T * {
        if (a == nullptr)
            return nullptr;
        if (a->type() == typeid(T))
            return &static_cast<detail::AnyDerive<std::remove_const_t<T>> *>(
                        a->ptr_.get())
                        ->value_;
        return nullptr;
    }

    template <class T, class DecayT = std::decay_t<T>>
        requires std::constructible_from<T, const DecayT &>
    friend auto any_cast(const any &a) -> T {
        if (typeid(DecayT) == a.type())
            return static_cast<detail::AnyDerive<DecayT> *>(a.ptr_.get())
                ->value_;
        throw bad_any_cast{};
    }

    template <class T, class DecayT = std::decay_t<T>>
        requires std::constructible_from<T, DecayT &>
    friend auto any_cast(any &a) -> T {
        if (typeid(DecayT) == a.type())
            return static_cast<detail::AnyDerive<DecayT> *>(a.ptr_.get())
                ->value_;
        throw bad_any_cast{};
    }

    template <class T, class DecayT = std::decay_t<T>>
        requires std::constructible_from<T, DecayT &&>
    friend auto any_cast(any &&a) -> T {
        if (typeid(DecayT) == a.type())
            return static_cast<detail::AnyDerive<DecayT> *>(a.ptr_.get())
                ->value_;
        throw bad_any_cast{};
    }
};

auto swap(any &lhs, any &rhs) noexcept -> void { lhs.swap(rhs); }

} // namespace mystd