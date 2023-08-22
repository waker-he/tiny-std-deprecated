
#include "span.hpp"
#include "vector.hpp"
#include <cassert>

using namespace mystd;

consteval auto test_span1() -> bool {
    // sizeof check
    static_assert(sizeof(span<int>) == sizeof(void *) * 2);
    static_assert(sizeof(span<int, 1>) == sizeof(void *));

    // default ctor
    span<int> s1;
    span<int, 0> s2;

    vector<int> vec;
    vec.emplace_back(0);
    vec.emplace_back(1);
    vec.emplace_back(2);

    // CTAD
    span s3{vec.data(), vec.size()};
    assert(sizeof s3 == sizeof(void *) * 2);
    assert(s3.size() == 3);
    for (std::size_t i = 0; i < s3.size(); i++)
        assert(s3[i] == static_cast<int>(i));

    // assignment
    s3 = span<int>(vec.data(), vec.data() + vec.size());
    assert(s3.size() == 3);
    for (std::size_t i = 0; i < s3.size(); i++)
        assert(s3[i] == static_cast<int>(i));

    // copy ctor
    auto s4{s3};
    assert(s4.size() == 3);
    for (std::size_t i = 0; i < s4.size(); i++)
        assert(s4[i] == static_cast<int>(i));

    s1 = s4;
    assert(s1.size() == 3);
    for (std::size_t i = 0; i < s1.size(); i++)
        assert(s1[i] == static_cast<int>(i));

    // subviews
    auto s5 = s1.first<2>();
    assert(sizeof s5 == sizeof(void *));
    assert(s5.size() == 2);
    assert(s5[0] == 0 && s5[1] == 1);

    // auto s6 = s5.first<3>(); // ERROR

    auto s7 = s1.first(2);
    assert(sizeof s7 == sizeof(void *) * 2);
    assert(s7.size() == 2);
    assert(s7[0] == 0 && s7[1] == 1);

    auto s8 = s1.last(2);
    assert(s8[0] == 1 && s8[1] == 2);
    assert(s8.size() == 2);

    auto s9 = s1.last<2>();
    assert(s9[0] == 1 && s9[1] == 2);
    assert(s9.size() == 2);

    return true;
}

auto main() -> int { static_assert(test_span1()); }