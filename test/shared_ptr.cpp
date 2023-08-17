
#include "memory.hpp"
#include <vector>
#include <cassert>
#include <thread>
#include <iostream>

using namespace mystd;

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
    Base() {
        counts++;
    }
    virtual ~Base() = default;
};
struct Derive : Base {
    double j;
    virtual ~Derive() {
        counts--;
    }
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

    {
        constexpr int NUM_THREADS = 32;
        constexpr int ITERATIONS = 100000;

        std::vector<std::thread> threads;
        for (int i = 0; i < NUM_THREADS; i++)
            threads.emplace_back(thread_func, ITERATIONS);
        
        for (auto& t : threads)
            t.join();
    }

    assert(counts == 0);
}

auto main() -> int {
    test_control_block();
    test_shared_ptr();
}