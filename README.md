# tiny-stl
- My C++ standard library implementation for practice purpose
- not intensively tested, not guaranteed to be robust

# implemented

## [any](./src/any.hpp)
- classic type erasure implementation
    - using __compiler-generated vtable__ instead of __manual vtable__
    - does not use __Small Object Optimization (SSO)__
        - just keeps an 8-byte `std::unique_ptr<AnyBase>`
    - affordances supported:
        - `clone()`: to support `any` construction and copy assignment
        - `type()`: to support `any::type()`

## [functional](./src/functional.hpp)