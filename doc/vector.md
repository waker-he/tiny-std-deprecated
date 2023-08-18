
# vector

- [`vector`](#vector-1)
- [`fixed_capacity_vector`](#fixed_capacity_vector)

## `vector`
- [code](../src/vector.hpp)
- do not have iterator implementation yet, `begin`, `end`... are undefined
- just implemented a set of basic functionalities, not cover all the standard interfaces
- not allocator-aware
- to enable `constexpr`:
    - cannot use `malloc` when allocating, instead, use:
        - `new char[]`
        - `std::allocator::allocate(std::size_t n)`
    - cannot use placement new, use `std::construct_at`
    - cannot use `std::memcpy`, `std::uninitailzed_move`, `std::uninitailzed_copy`
        - combine `construct_at` and `move_if_noexcept`
- potential optimization to do
    - modify `grow` when `std::is_trivially_relocatable` becomes available in the future
    - assignment
        - assume not self-assignment is common case, this implementation optimize for this common case and does not check if it is self
            - if assumption wrong, add this checking branch
        - copy assignment:
            - when `rhs` has a smaller capacity, we can save the extra allocation and deallocation by copying into the space that is already allocated


## `fixed_capacity_vector`
- [code](../src/fixed_capacity_vector.hpp)
- different from `array<T, N>`
    - `array` will start the lifetime for each of its elemnts when constructed, while `fixed_capacity_vector` won't
- usage in `constexpr` context:
    - if `std::is_trivially_destructible_v<T>` and `std::is_trivially_constructible_v<T>`
        - used `T[N]` for storage
        - can be used in `constexpr`
    - else
        - use `alignas(T) char[N * sizeof(T)]`
        - will need `reinterpret_cast` to access element
        - `reinterpret_cast` cannot be used in `constexpr` context since it need to be ensured that `constexpr` context does not have __undefined behavior__
    - can be made `constexpr` for all types if using dynamic allocation, but:
        - in this case accessing element will need an extra indirection
        - slower than stack allocation
