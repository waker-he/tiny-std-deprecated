# tiny-stl
- My C++ standard library implementation for practice purpose
- not intensively tested, not guaranteed to be robust

# implemented

- [`any`](#any)
- [`function_ref`](#function_ref)
- [Smart Pointers](#smart-pointers)
    - [`unique_ptr`](#unique_ptr)

# [`any`](./src/any.hpp)
- classic type erasure implementation
    - used __compiler-generated vtable__ instead of __manual vtable__
    - does not use __Small Object Optimization (SSO)__
        - just keeps an 8-byte `std::unique_ptr<AnyBase>`
    - affordances supported:
        - `clone()`: to support `any` construction and copy assignment
        - `type()`: to support `any::type()`

# [`function_ref`](./src/functional.hpp)

- practice a naive implementation of type erasure
    - no __compiler-generated vtable__ or __manual vtable__
    - `void *callable_ptr`: points to the bit representation of the callable
    - `ReturnType (*do_call_ptr)(void *, ArgTypes...)`: behavior of the only affordance supported: __invoke__
    - pros
        - fast, less extra indirection
    - cons
        - need one extra function pointer for each extra affordance
            - in this case, we just need one affordace so this approach works well
- class template argument deduction (CTAD)
    - uses deduction guide for:
        - function pointer
        - function object with no overloaded function call operators

# [Smart Pointers](./src/smart_pointers.hpp)

- does not contain implementation of array-version

## `unique_ptr`

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
    - do not declare copy ctor/assignment at all, even `= delete`:
        - `unique_ptr(const unique&) = delete;`
        - `unique_ptr& operator=(const unique&) = delete;`
        - declaring them will make them participate in overload resolution, and there is chance they are chosen over __move ctor/assignment__
    - `std::covertible_to<From*, To*>` does not guarantee `std::convertible_to<From, To>`
        - the pointer convertibility primarily concerns inheritance and polymorphism
        - direct type convertibility concerns constructors and conversion operators

