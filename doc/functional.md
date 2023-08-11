# functional

- [`function_ref`](#function_ref)

## `function_ref`

- [code](../src/functional.hpp)
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