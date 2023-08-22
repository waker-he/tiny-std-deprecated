#pragma once

#include "fixed_capacity_vector.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace mystd {

template <class T, std::size_t N> class small_size_optimized_vector {
  public:
    // member types
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = T *;
    using const_iterator = const T *;

    // constructors
    constexpr small_size_optimized_vector() noexcept
        : _data(storage_begin()), _sz(0), _cap(N) {}

    // copy ctor
    // will not copy capacity, the capacity will be max(other.size, N)
    constexpr small_size_optimized_vector(
        const small_size_optimized_vector &other)
        : _data(other._sz > N ? _alloc.allocate(other._sz) : storage_begin()),
          _sz(other._sz), _cap(std::max(N, other._sz)) {
        // copy elements
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(_data + i, *(other._data + i));
    }

    constexpr small_size_optimized_vector(
        small_size_optimized_vector &&other) noexcept {
        if (other._cap == N) {
            // move elements if in buffer
            _data = storage_begin();
            for (size_type i = 0; i < other._sz; i++)
                std::construct_at(_data + i, std::move(*(other._data + i)));
            other.destroy_all();
        } else {
            // just move pointer to heap memory
            _data = std::exchange(other._data, other.storage_begin());
            _cap = std::exchange(other._cap, N);
        }
        _sz = std::exchange(other._sz, 0);
    }

    // assignment, requires { T does not contain data member type
    // small_size_optimized_vector<T> }
    constexpr auto operator=(const small_size_optimized_vector &other)
        -> small_size_optimized_vector & {
        if (this == &other)
            return *this;
        destroy_all();
        if (_cap < other._sz) {
            // need heap allocation
            do_deallocate();
            _data = _alloc.allocate(other._sz);
            _cap = other._sz;
        }

        // copy elements
        for (size_type i = 0; i < other._sz; i++)
            std::construct_at(_data + i, *(other._data + i));
        _sz = other._sz;
        return *this;
    }

    constexpr auto operator=(small_size_optimized_vector &&other) noexcept
        -> small_size_optimized_vector & {
        if (this == &other)
            return *this;
        destroy_all();
        if (other._cap == N) {
            // move elements if in buffer
            // since this->cap >= other._cap, no reallocation is needed
            for (size_type i = 0; i < other._sz; i++)
                std::construct_at(_data + i, std::move(*(other._data + i)));
            other.destroy_all();
        } else {
            // just move pointer to heap memory
            do_deallocate();
            _data = std::exchange(other._data, other.storage_begin());
            _cap = std::exchange(other._cap, N);
        }
        _sz = std::exchange(other._sz, 0);
        return *this;
    }

    // destructor
    constexpr ~small_size_optimized_vector() {
        destroy_all();
        do_deallocate();
    }

    // element access
    constexpr auto data() const noexcept -> T * { return _data; }

    constexpr auto operator[](size_type i) noexcept -> reference {
        return *(_data + i);
    }
    constexpr auto operator[](size_type i) const noexcept -> const_reference {
        return *(_data + i);
    }

    // capacity
    constexpr auto empty() const noexcept -> bool { return _sz == 0; }
    constexpr auto size() const noexcept -> size_type { return _sz; }
    constexpr auto capacity() const noexcept -> size_type { return _cap; }
    constexpr auto reserve(size_type new_cap) -> void {
        if (new_cap > _cap)
            grow(new_cap);
    }

    // modifiers
    constexpr auto clear() noexcept -> void {
        destroy_all();
        _sz = 0;
    }

    constexpr auto swap(small_size_optimized_vector &other) noexcept -> void {
        std::swap(*this, other);
    }

    template <class... Args>
    constexpr auto emplace_back(Args &&...args) -> reference {
        if (_sz == _cap)
            grow(_cap * 2);
        return *std::construct_at(_data + _sz++, std::forward<Args>(args)...);
    }

    constexpr auto pop_back() noexcept -> void {
        std::destroy_at(_data + (--_sz));
    }

  private:
    using storage_type = std::conditional_t<detail::sufficiently_trivial<T>,
                                            T[N], char[N * sizeof(T)]>;
    T *_data;
    size_type _sz;
    size_type _cap;
    [[no_unique_address]] std::allocator<T> _alloc;
    [[no_unique_address]] alignas(T) storage_type _storage;

    constexpr auto storage_begin() noexcept -> T * {
        if constexpr (detail::sufficiently_trivial<T>) {
            return _storage;
        } else {
            return reinterpret_cast<T *>(_storage);
        }
    }

    constexpr auto do_deallocate() -> void {
        if (_cap > N)
            _alloc.deallocate(_data, _cap);
    }

    // input n should be greater than capacity
    constexpr auto grow(size_type n) -> void {
        auto new_data = _alloc.allocate(n);

        for (size_type i = 0; i < _sz; i++)
            std::construct_at(new_data + i,
                              std::move_if_noexcept(*(_data + i)));
        destroy_all();
        do_deallocate();

        _data = new_data;
        _cap = n;
    }

    constexpr auto destroy_all() const noexcept -> void {
        if constexpr (!std::is_trivially_destructible_v<T>) {
            for (auto first = _data, last = _data + _sz; first != last;
                 first++) {
                std::destroy_at(first);
            }
        }
    }
};

} // namespace mystd
