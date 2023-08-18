
#include "memory.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace mystd;
using namespace std::literals;

std::atomic<int> counts{0};

auto test_control_block() -> void {
    detail::control_block_with_obj<int> a;
    a.emplace(2);
    assert(a.shared_count == 1);
    assert(a.weak_count == 1);

    detail::control_block_with_ptr<int> c2;
    assert(c2.ptr == nullptr);
}

struct Base {
    int i;
    Base() { counts++; }
    virtual ~Base() = default;
};
struct Derive : Base {
    double j;
    virtual ~Derive() { counts--; }
};

void thread_func(const int n) {
    for (int i = 0; i < n; i++) {
        auto sp1{make_shared<Derive>()};
        auto sp2 = sp1;
        auto sp3 = sp2;
    }
}

auto test_shared_ptr() -> void {
    {
        shared_ptr<Derive> sp1(new Derive{});
        shared_ptr<Base> sp2(std::move(sp1));
        assert(sp2->i == 0);
        shared_ptr<int> sp3(std::move(sp2), &sp2->i);
        assert(*sp3 == 0);
        assert(sp3.use_count() == 1);
        assert(sp2.use_count() == 0);
        assert(sp1.use_count() == 0);
    }
    assert(count2 == 0);

    {
        // make_shared
        auto sp1{make_shared<Derive>()};
        shared_ptr<Base> sp2(std::move(sp1));
        assert(sp2->i == 0);
        assert(sp2 != nullptr);
        shared_ptr<int> sp3(std::move(sp2), &sp2->i);
        assert(*sp3 == 0);
        assert(sp3.use_count() == 1);
        assert(sp2.use_count() == 0);
        assert(sp1.use_count() == 0);

        auto sp4{sp3};
        assert(sp3.use_count() == 2);
        assert(sp3 == sp4);
        auto sp5{std::move(sp3)};
        assert(sp3 != sp4);
        assert(sp3.use_count() == 0);
        assert(sp4.use_count() == 2);
        assert(sp5.use_count() == 2);

        // convert from unique_ptr
        auto up{make_unique<Derive>()};
        shared_ptr sp6{std::move(up)};
    }
    assert(count2 == 0);

    {
        // assignments
        shared_ptr<Derive> sp1(new Derive{});
        sp1 = sp1;
        assert(sp1.use_count() == 1);
        shared_ptr<Base> sp2(new Derive{});
        sp2 = sp1;
        assert(sp1.use_count() == 2);
        sp2 = sp1;
        assert(sp1.use_count() == 2);
        assert(sp2.use_count() == 2);

        auto up(make_unique<Derive>());
        sp2 = std::move(up);
        assert(sp2.use_count() == 1);
        assert(sp1.use_count() == 1);
    }
    assert(count2 == 0);

    {
        constexpr int NUM_THREADS = 32;
        constexpr int ITERATIONS = 100000;

        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; i++)
            threads.emplace_back(thread_func, ITERATIONS);

        for (auto &t : threads)
            t.join();
    }
    assert(count2 == 0);
    assert(counts == 0);
}

auto test_weak_ptr() -> void {
    {
        // default construction
        weak_ptr<Base> wp;
        assert(wp.expired());
        assert(wp.use_count() == 0);
    }

    {
        // construction from shared_ptr
        shared_ptr<Base> sp = make_shared<Derive>();
        assert(counts == 1);

        weak_ptr<Base> wp(sp);
        assert(!wp.expired());
        assert(wp.use_count() == 1);

        sp.reset();
        assert(wp.expired());
        assert(wp.use_count() == 0);
    }
    assert(counts == 0);
    assert(count2 == 0);

    {
        // locking and converting to shared_ptr
        shared_ptr<Base> sp = make_shared<Derive>();
        weak_ptr<Base> wp(sp);

        auto locked_sp = wp.lock();
        assert(locked_sp);
        assert(wp.use_count() == 2);

        sp.reset();
        locked_sp.reset();
        assert(wp.expired());
    }
    assert(counts == 0);

    {
        // copy and move constructions
        shared_ptr<Derive> sp = make_shared<Derive>();
        weak_ptr<Derive> wp1(sp);

        weak_ptr<Derive> wp2(wp1); // Copy
        assert(wp2.use_count() == 1);

        weak_ptr<Base> wp3(std::move(wp2)); // Move
        assert(wp3.use_count() == 1);
        assert(wp2.expired());
    }
    assert(counts == 0);
    assert(count2 == 0);

    {
        // assignments
        shared_ptr<Derive> sp1 = make_shared<Derive>();
        shared_ptr<Derive> sp2 = make_shared<Derive>();
        weak_ptr<Derive> wp1(sp1);
        weak_ptr<Derive> wp2(sp2);

        wp1 = wp2; // Assign from weak_ptr
        assert(wp1.use_count() == 1);

        wp1 = sp1; // Assign from shared_ptr
        assert(wp1.use_count() == 1);
    }
    assert(counts == 0);
    assert(count2 == 0);

    {
        // multithreading
        constexpr int thread_count = 5;
        constexpr int iteration_count = 1000;
        std::atomic<int> successful_locks{0};

        shared_ptr<Base> sp = make_shared<Derive>();
        weak_ptr<Base> wp(sp);
        std::vector<std::thread> threads;
        for (int i = 0; i < thread_count; ++i) {
            threads.emplace_back([&]() {
                for (int j = 0; j < iteration_count; ++j) {
                    if (auto locked_sp = wp.lock()) {
                        ++successful_locks;
                    }
                }
            });
        }

        // Simulate a scenario where the shared pointer may be reset by another
        // thread
        std::this_thread::sleep_for(200ms);
        sp.reset();

        for (auto &t : threads) {
            t.join();
        }

        // Print out the number of successful locks
        std::cout << "Successful locks: " << successful_locks.load()
                  << std::endl;
    }
    assert(counts == 0);
    assert(count2 == 0);
}

// ****************************************************************************
// *                 enable_shared_from_this test                             *
// ****************************************************************************

struct TestESSFT : public enable_shared_from_this<TestESSFT> {
    int value = 42;
};

void basic_test() {
    auto ptr = make_shared<TestESSFT>();
    auto shared_from_this = ptr->shared_from_this();

    assert(shared_from_this->value == 42);
    assert(shared_from_this.get() == ptr.get());
    assert(shared_from_this.use_count() == 2);

    auto weak_from_this = ptr->weak_from_this();
    assert(weak_from_this.lock().get() == ptr.get());
}

void constructor_test() {
    TestESSFT *rawPtr = new TestESSFT();
    shared_ptr<TestESSFT> ptr1(rawPtr);

    auto shared_from_raw = rawPtr->shared_from_this();
    assert(shared_from_raw.get() == ptr1.get());
    assert(shared_from_raw.use_count() == 2);

    shared_ptr<TestESSFT> ptr2(
        rawPtr); // According to standard, this is undefined behavior. It's a
                 // bad practice but for the sake of testing, we're doing it
                 // here.

    auto shared_from_raw_after = rawPtr->shared_from_this();
    assert(shared_from_raw_after.get() == ptr2.get());
    assert(shared_from_raw_after.use_count() == 2);

    // segfault when double deleting
}

void exception_test() {
    TestESSFT obj;

    try {
        auto bad_shared = obj.shared_from_this();
        assert(false); // should not reach here
    } catch (const bad_weak_ptr &) {
        // Expected: shared_from_this should throw if no shared_ptr owns obj
    }
}

struct DerivedESSFT : public TestESSFT {};
static_assert(detail::inherits_from_enable_shared_from_this<DerivedESSFT>);

void derived_test() {
    auto ptr = make_shared<DerivedESSFT>();
    shared_ptr<TestESSFT> basePtr = ptr;
    assert(ptr.use_count() == 2);

    auto derived_shared_from_this = ptr->shared_from_this();
    assert(ptr.use_count() == 3);
    assert(derived_shared_from_this.get() == ptr.get());

    auto base_shared_from_this = basePtr->shared_from_this();
    assert(ptr.use_count() == 4);
    assert(base_shared_from_this.get() == basePtr.get());
}

auto main() -> int {
    test_control_block();
    test_shared_ptr();
    test_weak_ptr();

    // enable_shared_from_this
    basic_test();
    std::cout << "pass enable_shared_from_this basic test\n";
    // constructor_test();
    // std::cout << "pass enable_shared_from_this ctor test\n";
    exception_test();
    std::cout << "pass enable_shared_from_this exception test\n";
    derived_test();
    std::cout << "pass enable_shared_from_this derived test\n";
}