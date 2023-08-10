
#include "smart_pointers.hpp"
#include <iostream>

using namespace mystl;

struct S {
    S() { std::cout << "ctor\n"; }

    ~S() { std::cout << "dtor\n"; }

    S(const S &) { std::cout << "copy ctor\n"; }

    S(S &&) noexcept { std::cout << "move ctor\n"; }

    auto operator=(const S &) -> S & {
        std::cout << "copy assign\n";
        return *this;
    }

    auto operator=(S &&) noexcept -> S & {
        std::cout << "move assign\n";
        return *this;
    }
};

auto test_default_delete1() -> void {
    default_delete<S[]> dd1;
    S *s_arr = ::new S[3];
    dd1(s_arr);

    default_delete<S> dd2;
    S *s = ::new S{};
    dd2(s);
}

consteval auto test_default_delete2() -> bool {
    int* i = new int{3};
    default_delete<int> dd;
    dd(i);
    return true;
}

auto main() -> int {
    test_default_delete1();
    static_assert(test_default_delete2());
}