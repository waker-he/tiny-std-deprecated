
# vector

- [`vector`](./vector.md)

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