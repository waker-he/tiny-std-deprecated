
# `any`

- [code](../src/any.hpp)
- classic type erasure implementation
    - used __compiler-generated vtable__ instead of __manual vtable__
    - does not use __Small Object Optimization (SSO)__
        - just keeps an 8-byte `mystd::unique_ptr<AnyBase>`
    - affordances supported:
        - `clone()`: to support `any` construction and copy assignment
        - `type()`: to support `any::type()`
- it requires the value type to be `std::copy_constructible`, since `any` does not have template parameters and cannot decide whether to define copy ctors according to the holding type
