#pragma once

#include "fixed_capacity_vector.hpp"
#include "small_size_optimized_vector.hpp"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

namespace mystd {

template <class T> class vector {
  public:
    // member types
    using value_type = T;
    using size_type = std::size_t;
    using reference = value_type &;
    using const_reference = const value_type &;

    // constructors
    constexpr vector() noexcept : _data{nullptr}, _sz{0}, _cap{0} {}
    constexpr vector(const vector &other) : _sz{other._sz}, _cap{other._cap} {
        _data = _alloc.allocate(_cap);
        // copy elements
        for (size_type i = 0; i < _sz; i++)
            std::construct_at(_data + i, *(other._data + i));
    }
    constexpr vector(vector &&other) noexcept
        : _data{std::exchange(other._data, nullptr)},
          _sz{std::exchange(other._sz, 0)}, _cap{std::exchange(other._cap, 0)} {
    }

    // assignment
    constexpr auto operator=(vector rhs) -> vector & {
        swap(rhs);
        return *this;
    }

    // destructor
    constexpr ~vector() {
        destroy_all();
        do_deallocate();
    }

    // element access
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

    constexpr auto swap(vector &other) noexcept -> void {
        std::swap(_data, other._data);
        std::swap(_sz, other._sz);
        std::swap(_cap, other._cap);
    }

    template <class... Args>
    constexpr auto emplace_back(Args &&...args) -> reference {
        if (_sz == _cap)
            grow(_cap ? _cap * 2 : 1);
        return *std::construct_at(_data + _sz++, std::forward<Args>(args)...);
    }

    constexpr auto pop_back() noexcept -> void {
        std::destroy_at(_data + (--_sz));
    }

  private:
    T *_data;
    size_type _sz;
    size_type _cap;
    [[no_unique_address]] std::allocator<T> _alloc;

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

    constexpr auto do_deallocate() -> void {
        if (_cap > 0)
            _alloc.deallocate(_data, _cap);
    }
};

} // namespace mystd
