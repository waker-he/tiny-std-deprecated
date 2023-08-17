#include "any.hpp"
#include <cassert>
#include <vector>

using namespace mystd;

auto main() -> int {
    // ctor
    any a1;
    any a2 = 3.14;
    any a3 = a2;
    any a4 = std::move(a2);
    any a5 = a1;
    a5 = a5;

    assert(!a1.has_value());
    assert(!a2.has_value());
    assert(a3.has_value());
    assert(a4.has_value());

    double *p1 = any_cast<double>(&a3);
    float *p2 = any_cast<float>(&a3);
    assert(*p1 == 3.14);
    assert(p2 == nullptr);

    double v1 = any_cast<double>(a4);
    assert(v1 == 3.14);
    a4 = 4;
    assert(any_cast<int>(a4) == 4);

    try {
        any_cast<double>(a4);
    } catch (bad_any_cast &e) {
    } catch (...) {
        assert(false);
    }

    a1.emplace<std::vector<int>>({1, 2});
    assert(any_cast<std::vector<int>>(a1).size() == 2);
    assert(any_cast<std::vector<int>>(a1)[0] == 1);
    assert(any_cast<std::vector<int>>(a1)[1] == 2);
    a1.emplace<std::vector<int>>(1, 2);
    assert(any_cast<std::vector<int>>(a1)[0] == 2);
    assert(any_cast<std::vector<int>>(a1).size() == 1);
    a1.reset();
    assert(!a1.has_value());
}