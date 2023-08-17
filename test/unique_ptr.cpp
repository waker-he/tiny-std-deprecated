
#include "memory.hpp"
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

    auto operator()(int *ptr) const -> void { ::delete ptr; }
};

// test detail namespace
static_assert(detail::deleter_copy_constructible_to<S &, S &>);
static_assert(detail::deleter_copy_constructible_to<S &, S>);
static_assert(!detail::deleter_copy_constructible_to<S, S &>);
static_assert(!detail::deleter_copy_constructible_to<S, S>);

static_assert(!detail::deleter_move_constructible_to<S &, S &>);
static_assert(!detail::deleter_move_constructible_to<S &, S>);
static_assert(!detail::deleter_move_constructible_to<S, S &>);
static_assert(detail::deleter_move_constructible_to<S, S>);

static_assert(detail::deleter_copy_assignable_to<S &, S &>);
static_assert(detail::deleter_copy_assignable_to<S &, S>);
static_assert(!detail::deleter_copy_assignable_to<S, S &>);
static_assert(!detail::deleter_copy_assignable_to<S, S>);

static_assert(!detail::deleter_move_assignable_to<S &, S &>);
static_assert(!detail::deleter_move_assignable_to<S &, S>);
static_assert(detail::deleter_move_assignable_to<S, S &>);
static_assert(detail::deleter_move_assignable_to<S, S>);

auto test_default_delete1() -> void {
    std::cout << "test default_delete:\n";
    default_delete<S[]> dd1;
    S *s_arr = ::new S[3];
    dd1(s_arr);

    default_delete<S> dd2;
    S *s = ::new S{};
    dd2(s);
}

consteval auto test_default_delete2() -> bool {
    int *i = new int{3};
    default_delete<int> dd;
    dd(i);
    return true;
}

struct Base {
    virtual ~Base() = default;
};
struct Derived : Base {
    virtual ~Derived() = default;
};

consteval auto test_unique_ptr1() -> bool {
    static_assert(sizeof(unique_ptr<int, S>) == sizeof(void *));
    static_assert(sizeof(unique_ptr<int, S &>) == 2 * sizeof(void *));
    // normal ctors
    unique_ptr<int> ptr1(new int{2});
    assert(*ptr1 == 2);
    auto ptr2(std::move(ptr1));
    assert(!static_cast<bool>(ptr1));
    assert(static_cast<bool>(ptr2));

    // assign to nullptr
    ptr2 = nullptr;
    assert(!static_cast<bool>(ptr2));

    // default ctors
    unique_ptr<double> ptr3;
    unique_ptr<double> ptr4(nullptr);
    assert(!static_cast<bool>(ptr3));
    assert(!static_cast<bool>(ptr4));

    // move assignments
    unique_ptr<int> ptr5(new int{2});
    ptr2 = std::move(ptr5);
    assert(static_cast<bool>(ptr2));
    assert(!static_cast<bool>(ptr5));
    assert(*ptr2 == 2);
    ptr2 = std::move(ptr2);
    assert(static_cast<bool>(ptr2));
    assert(*ptr2 == 2);

    // swap
    auto ptr6 = make_unique<int>(6);
    ptr6.swap(ptr2);
    assert(*ptr2 == 6);
    assert(*ptr6 == 2);

    // comparator
    assert(ptr2 != ptr6);
    assert(ptr2 == ptr2);
    assert(ptr2 != nullptr);
    assert(nullptr != ptr2);
    assert(ptr1 == nullptr);

    return true;
}

auto test_unique_ptr2() -> void {
    std::cout << "test unique_ptr:\n";
    unique_ptr<Derived> ptr1(new Derived{});
    assert(static_cast<bool>(ptr1));
    unique_ptr<Base> ptr2(std::move(ptr1));
    assert(!static_cast<bool>(ptr1));
    assert(static_cast<bool>(ptr2));

    assert(ptr1 < ptr2 || ptr1 >= ptr2);

    unique_ptr<int, S> ptr3(new int{3});
    S s1;
    unique_ptr<int, S &> ptr4(new int{4}, s1);
    assert(ptr3 != ptr4);
    unique_ptr<int, const S &> ptr5(new int{5}, s1);
    assert(*ptr5 == 5);
}

auto main() -> int {
    test_default_delete1();
    static_assert(test_default_delete2());
    static_assert(test_unique_ptr1());
    test_unique_ptr2();
}