# memory

- [`unique_ptr`](#unique_ptr)

## `unique_ptr`

- [code](../src/smart_pointers/unique_ptr.hpp)
- do not have implementation of array-version
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
    - `std::covertible_to<From*, To*>` is independent from `std::convertible_to<From, To>`
        - the pointer convertibility primarily concerns inheritance and polymorphism
        - direct type convertibility concerns constructors and conversion operators