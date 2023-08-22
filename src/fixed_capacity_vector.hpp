#pragma once

#include <memory>
#include <type_traits>

namespace mystd {

namespace detail {

template <class T>
concept sufficiently_trivial =
    std::is_trivially_destructible_v<T> && std::is_trivially_constructible_v<T>;
}

template <class T, std::size_t N> class fixed_capacity_vector {
  public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = T *;
    using const_iterator = const T *;

    // constructors
    constexpr fixed_capacity_vector() noexcept : _sz{0} {}
    constexpr fixed_capacity_vector(const fixed_capacity_vector &other)
        : _sz{other._sz} {
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(begin() + i, other[i]);
    }

    constexpr fixed_capacity_vector(fixed_capacity_vector &&other) noexcept
        : _sz{other._sz} {
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(begin() + i, std::move(other[i]));
        other._sz = 0;
    }

    // assignments
    constexpr auto operator=(const fixed_capacity_vector &other)
        -> fixed_capacity_vector & {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            destroy_all();
        }
        _sz = other._sz;
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(begin() + i, other[i]);
        return *this;
    }

    constexpr auto operator=(fixed_capacity_vector &&other)
        -> fixed_capacity_vector & {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            destroy_all();
        }
        _sz = other._sz;
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(begin() + i, std::move(other[i]));
        other._sz = 0;
        return *this;
    }

    // destructor
    constexpr ~fixed_capacity_vector() {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            destroy_all();
        }
    }

    // element access
    constexpr auto data() const noexcept -> T * { return begin(); }

    constexpr auto begin() noexcept -> iterator {
        if constexpr (detail::sufficiently_trivial<T>) {
            return _storage;
        } else {
            return reinterpret_cast<T *>(_storage);
        }
    }

    constexpr auto begin() const noexcept -> const_iterator {
        if constexpr (detail::sufficiently_trivial<T>) {
            return _storage;
        } else {
            return reinterpret_cast<const T *>(_storage);
        }
    }

    constexpr auto end() noexcept -> iterator { return begin() + _sz; }

    constexpr auto end() const noexcept -> const_iterator {
        return begin() + _sz;
    }

    constexpr auto operator[](size_type pos) noexcept -> reference {
        return *(begin() + pos);
    }

    constexpr auto operator[](size_type pos) const noexcept -> const_reference {
        return *(begin() + pos);
    }

    // capacity
    constexpr auto size() const noexcept -> size_type { return _sz; }
    constexpr auto capacity() const noexcept -> size_type { return N; }
    constexpr auto empty() const noexcept -> size_type { return _sz == 0; }

    // modifiers
    constexpr auto clear() noexcept -> void {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            destroy_all();
        }
        _sz = 0;
    }

    template <class... Args>
    constexpr auto emplace_back(Args &&...args) -> reference {
        return *std::construct_at(begin() + _sz++, std::forward<Args>(args)...);
    }

    constexpr auto pop_back() noexcept -> void {
        --_sz;
        if constexpr (!std::is_trivially_destructible_v<T>) {
            std::destroy_at(end());
        }
    }

  private:
    using storage_type = std::conditional_t<detail::sufficiently_trivial<T>,
                                            T[N], char[N * sizeof(T)]>;
    alignas(T) storage_type _storage;
    size_type _sz;

    auto destroy_all() const noexcept -> void {
        for (auto first = begin(), last = end(); first != last; first++) {
            std::destroy_at(first);
        }
    }
};
} // namespace mystd