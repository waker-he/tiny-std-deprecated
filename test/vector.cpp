
#include "vector.hpp"
#include <cassert>
#include <iostream>
using namespace mystd;

struct S {
    S() { std::cout << "ctor\n"; }

    ~S() { std::cout << "dtor\n"; }

    S(const S &) noexcept { std::cout << "copy ctor\n"; }

    S(S &&) noexcept { std::cout << "move ctor\n"; }

    auto operator=(const S &) noexcept -> S & {
        std::cout << "copy assign\n";
        return *this;
    }

    auto operator=(S &&) noexcept -> S & {
        std::cout << "move assign\n";
        return *this;
    }
};

consteval auto test_vector1() -> bool {
    vector<int> vec;
    assert(vec.empty());
    vec.emplace_back(2);
    assert(vec.capacity() == 1);
    assert(vec.size() == 1);
    int j = 3;
    vec.emplace_back(j);
    assert(vec[0] == 2 && vec[1] == 3);

    vec.clear();
    assert(vec.empty());
    assert(vec.capacity() == 2);

    vec.emplace_back(15);
    auto vec2 = vec;
    auto vec3 = std::move(vec);
    assert(vec.empty());
    assert(vec2[0] == vec3[0]);
    assert(vec2[0] == 15);

    vec3[0] = 23;
    vec2 = vec3;
    assert(vec2.size() == vec3.size());
    assert(vec2[0] == 23);
    return true;
}

auto test_vector2() -> void {
    vector<S> vec;
    vec.emplace_back();
    vec.emplace_back(S{});
    vec.pop_back();
    vec.reserve(100);

    S s1;
    vec.emplace_back(s1);
    vec.emplace_back(std::move(s1));
}

consteval auto test_fixed_capacity_vector1() -> bool {
    fixed_capacity_vector<int, 5> vec;
    assert(vec.empty());
    vec.emplace_back(2);
    assert(vec.capacity() == 5);
    assert(vec.size() == 1);
    int j = 3;
    vec.emplace_back(j);
    assert(vec[0] == 2 && vec[1] == 3);

    vec.clear();
    assert(vec.empty());
    assert(vec.capacity() == 5);

    vec.emplace_back(15);
    auto vec2 = vec;
    auto vec3 = std::move(vec);
    assert(vec.empty());
    assert(vec2[0] == vec3[0]);
    assert(vec2[0] == 15);

    vec3[0] = 23;
    vec2 = vec3;
    assert(vec2.size() == vec3.size());
    assert(vec2[0] == 23);
    return true;
}

auto test_fixed_capacity_vector2() -> void {
    fixed_capacity_vector<S, 10> vec;
    vec.emplace_back();
    vec.emplace_back(S{});
    vec.pop_back();

    S s1;
    vec.emplace_back(s1);
    vec.emplace_back(std::move(s1));
}

auto main() -> int {
    std::cout << "test vector:\n";
    static_assert(test_vector1());
    test_vector2();
    std::cout << "test fixed_capacity_vector:\n";
    static_assert(test_fixed_capacity_vector1());
    test_fixed_capacity_vector2();
}