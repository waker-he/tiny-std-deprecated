# memory

- [`unique_ptr`](#unique_ptr)
- [`shared_ptr`](#shared_ptr)
- [`weak_ptr`](#weak_ptr)
- [`enable_shared_from_this`](#enable_shared_from_this)

## `unique_ptr`

- [code](../src/smart_pointers/unique_ptr.hpp)
- `mystd::unique_ptr` does not have array-version
- rules of propagating deleters from `From rhs` to `To lhs`, referred from https://en.cppreference.com/w/cpp/memory/unique_ptr/unique_ptr
    - if `From` and `To` are references
        - `lhs` is copy constructed/assigned from `rhs`
            - for copy construction, we are just constructing a reference
            - but for copy assignment, we are not assigning to the reference, we are assigning the object the reference refers to
    - if `From` is reference but `To` is not reference
        - `lhs` is copy constructed/assigned from `rhs`
    - if `From` is not reference but `To` is reference
        - __construction__: not allowed
        - __assignment__: move assigned from `rhs` to `lhs`
            - move assigned to the object `lhs` refers to, instead of `lhs` itself
    - if `From` and `To` are not reference
        - `lhs` is move constructed/assigned from `rhs`
- interesting bugs:
    - don't declare copy ctor/assignment at all, even `= delete`:
        - `unique_ptr(const unique&) = delete;`
        - `unique_ptr& operator=(const unique&) = delete;`
        - declaring them will make them participate in overload resolution, and there is chance they are chosen over __move ctor/assignment__
    - `std::covertible_to<From*, To*>` is independent from `std::convertible_to<From, To>`
        - the pointer convertibility primarily concerns inheritance and polymorphism
        - direct type convertibility concerns constructors and conversion operators

## `shared_ptr`

- [code](../src/smart_pointers/shared_ptr.hpp)
- `mystd::shared_ptr` does not have array-version
- in `std::shared_ptr` with customized __allocator__ and __deleter__:
    - __allocator__ is used to allocate and deallocate __control block__
    - __deleter__ is used to destroy and deallocate __managed object__
    - when `std::shared_ptr` is created using `std::make_shared` or `std::allocate_shared`
        - managed object is allocated in the control block
        - cannot specify custom __deleter__
    - no conflicts between __allocator__ and __deleter__ can occur
- `mystd::shared_ptr` do not have option for specifying __custom allocator__, __reasons__:
    - inherently conflicting model of `shared_ptr` with __custom allocator__
        - to deallocate control block, need to
            1. call the destructor of control block
            2. get the allocator that allocates the control block
        - the allocator is stored inside the control block, but it is destroyed in the first step
        - things get messy with allocator that is __stateful__ or __has side effects when beginning and ending its lifetime__
        - check [example](https://godbolt.org/z/63c3xb7c7): a single instance of `std::shared_ptr<int>` calls destructor of its __allocator__ for 6 times!
    - I don't like C++ STL allocator model
        - `std::allocator` sucks with this __rebinding__ stuff and its __type parameter__ does not make any sense
        - `std::pmr::polymorphic_allocator` is easier to use but requires 8 bytes of storage even using `new` and `delete`
        - [my allocator learning note](https://github.com/waker-umich/cs-learning-notes/blob/main/cpp/cppcon/allocator/allocator.md)
- about atomic operations on reference counts:
    - refer to
        - [my memory model note](https://github.com/waker-umich/cs-learning-notes/blob/main/cpp/concurrency/memory-model/memory-model.md)
        - [a well-explained answer on relaxed atomic usage for `shared_ptr` on stack overflow](https://stackoverflow.com/questions/48124031/stdmemory-order-relaxed-atomicity-with-respect-to-the-same-atomic-variable/48148318#48148318)
    - used `std::memory_order_relaxed` for incrementing
        - once control block is constructed, the increment order does not matter
        - invoking of copy constructors always have a `happens-before` relationship
    - for decrementing:
        ```cpp
        if (shared_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            delete ptr;
        }
        ```
        - technically, only the thread which modifies the shared object need to do a __release store__, and only the last thread that decrement the `shared_count` need an __acquire load__
        - but thread actions differ in each run, so we used `std::memory_order_acq_rel` for all decrements
        - we can optimize it to be:
            ```cpp
            if (shared_count.fetch_sub(1, std::memory_order_release) == 1) {
                std::atomic_thread_fence(std::memory_order_acquire);
                delete ptr;
            }
            ```
- fixed bug: if only define templated copy/move constructors and assignments, when copying from the same type, compiler-generated copy ctor/assignment will be called, which will not increment `shared_count`

## `weak_ptr`

- [code](../src/smart_pointers/shared_ptr.hpp)
- `weak_ptr` is an augmentation to `shared_ptr` and it is a ticket to `shared_ptr`
- to synchronize between `shared_ptr` and `weak_ptr` to ensure destorying control block only once
    - `weak_count` is defined as `#weak_ptr + (#shared_ptr != 0)`
- use lock-free add-if-not-zero operation for `lock()` implementation:
    ```cpp
    auto lock() const noexcept -> shared_ptr<T> {
        shared_ptr<T> sp{};
        if (_cb_ptr) {
            // perform lock-free add-if-not-zero operation
            auto count = _cb_ptr->shared_count.load(std::memory_order_relaxed);
            do {
                if (count == 0)
                    return sp;
            } while (_cb_ptr->shared_count.compare_exchange_weak(
                count, count + 1, std::memory_order_relaxed));
            // use std::memory_order_relaxed here because there is nothing to
            // acquire or release

            sp._ptr = _ptr;
            sp._cb_ptr = _cb_ptr;
        }

        return sp;
    }
    ```

## `enable_shared_from_this`

- [code](../src/smart_pointers/shared_ptr.hpp)
- implemented using `weak_ptr` data member, which is a _ticket_ to `shared_ptr`
- inheriting (`public`ly) will enable an object to take part in its own life time management
- constructing a `shared_ptr` for an object that is already managed by another `shared_ptr` is undefined behavior
- TO_DO: use C++23 deducing this to eliminate base class template parameter, change weak_ptr data member to a storage of `sizeof(weak_ptr)` and `alignof(weak_ptr)`, manually constructing and destructing the weak_ptr
