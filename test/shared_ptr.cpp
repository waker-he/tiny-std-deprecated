
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
        constexpr int iteration_count = 10000;
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
        std::this_thread::sleep_for(2s);
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

auto main() -> int {
    test_control_block();
    test_shared_ptr();
    test_weak_ptr();
}