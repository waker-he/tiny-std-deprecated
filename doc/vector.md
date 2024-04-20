
# vector

- [`vector`](#vector-1)
- [`fixed_capacity_vector`](#fixed_capacity_vector)
- [`small_size_optimized_vector`](#small_size_optimized_vectort-n)

## `vector`
- [code](../src/vector.hpp)
- do not have iterator implementation yet, `begin`, `end`... are undefined
- just implemented a set of basic functionalities, not cover all the standard interfaces
- not allocator-aware
- for implementation to enable usage in `constexpr`:
    - cannot use `malloc` when allocating, instead, use `std::allocator<T>::allocate(std::size_t n)`
        - reason:
           - `std::allocator<T>::allocate(std::size_t n)` returns `T *`, which is a typed pointer
           - `malloc` or `::operator new` returns `void *`
    - cannot use placement new, use `std::construct_at`
    - cannot use `std::memcpy`, `std::uninitailzed_move`, `std::uninitailzed_copy`, use `construct_at` and `move_if_noexcept`
- potential optimization to do
    - modify `grow` when `std::is_trivially_relocatable` becomes available in the future
    - assignment
        - assume not self-assignment is common case, this implementation optimize for this common case and does not check if it is self
            - if assumption wrong, add this checking branch
        - copy assignment:
            - when `rhs.size() <= this->capacity()`, we can save the extra allocation and deallocation by copying into the space that is already allocated
                - this optimization can only be done if `T` is not a recursive data structure, i.e., `T` does not have data member `vector<T>`
                    ```cpp
                    // this data structure does not work with copy assignment optimization
                    struct Recur {
                        vector<Recur> vec;
                    };
                    ```
                - it seems that there is no known techniques to check it, so `mystd::vector` does not do this optimization, but for practice purpose [`mystd::small_size_optimized_vector`](#small_size_optimized_vectort-n) implements it
- __TO DO__:
    - exception safety: if there is exception when propagating elements in `grow`, need to restore the original state (destory and deallocate objects in new storage) and rethrow exception


## `fixed_capacity_vector`
- [code](../src/fixed_capacity_vector.hpp)
- comparisons to `vector`:
    - pros
        - one less indirection for accessing element
        - do not need branch that checks whether `size() == capacity()` when `emplace_back`
        - allocating elements on stack is faster and more cache-friendly
    - cons
        - more expensive for move operations
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
    - can be made `constexpr` for all types if using __dynamic allocation__, but:
        - in this case accessing element will need an extra indirection
        - slower than stack allocation
        - use memory on stack can have more cache hits

## `small_size_optimized_vector<T, N>`

- [code](../src/vector.hpp)
- if `size() <= N`, store elements in the `buffer`, else, allocate and store on the heap
- difference from `mystd::vector<T>`
    - one extra data member for `buffer`, where `sizeof buffer == sizeof(T) * N`
        - the type of this `buffer` is same as the storage type used in `fixed_capacity_vector`, so `constexpr` usage is the same as `fixed_capacity_vector`
    - member functions
        - speical member functions
        - `swap`: simply delegates to `std::swap(*this, other)`
            - consequence: cannot use __copy-swap idiom__ for assignment, also even if we can use, it can be quite inefficient
        - `do_deallocate`: simply change deallocation condition from `capacity() > 0` to `capacity() > N`
    - if inheriting from `mystd::vector<T>` and making functions that need to override `virtual`, we can pass in `mystd::small_size_optimized_vector<T, N>` where `mystd::vector<T>` is expected (by reference or pointer)
        - why this implementation is not chosen:
            - cost
                - `sizeof(void *)` extra bytes for storage
                - `virtual` member function calls cannot be resolved at compile-time
            - prefer `span` when need to pass `vector`s by reference, [`span`](./span.md) can be created as a view into all contiguous ranges
    - for copy assignment, if `this->capacity() >= rhs.size()`, this implementation does not do extra allocation and deallocation 
        - this is not a safe optimization as mentioned in [`vector`](#vector), if the `element_type` contains data member of type `small_size_optimized_vector<element_type>`, it will lead to __undefined behavior__
