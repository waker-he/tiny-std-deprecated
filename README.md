# tiny-stl
- My C++ standard library implementation for practice purpose
- not intensively tested, not guaranteed to be robust

# implemented

- [any](#any)
- [function_ref](#function_ref)

# [any](./src/any.hpp)
- classic type erasure implementation
    - used __compiler-generated vtable__ instead of __manual vtable__
    - does not use __Small Object Optimization (SSO)__
        - just keeps an 8-byte `std::unique_ptr<AnyBase>`
    - affordances supported:
        - `clone()`: to support `any` construction and copy assignment
        - `type()`: to support `any::type()`

# [function_ref](./src/functional.hpp)

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