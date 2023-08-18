#pragma once

#include "unique_ptr.hpp"
#include <array>
#include <atomic>
#include <exception>

// count2 is only for testing purpose
inline std::atomic<int> count2{0};

namespace mystd {

class bad_weak_ptr : public std::exception {
    virtual const char *what() const noexcept { return "bad weak_ptr"; }
};

// forward declaration
template <class T> class shared_ptr;

template <class T> class weak_ptr;

template <class T> class enable_shared_from_this;

namespace detail {

template <class T>
concept inherits_from_enable_shared_from_this =
    requires(T t) {
        t.shared_from_this();
    };

// ****************************************************************************
// *                              control_block                               *
// ****************************************************************************

struct control_block_base {
    std::atomic<std::size_t> shared_count = 1; // #shared
    std::atomic<std::size_t> weak_count = 1;   // #weak + (#shared != 0)
    virtual auto delete_obj() -> void = 0;

    control_block_base() { count2++; }
    virtual ~control_block_base() { count2--; }
    void decrement_shared() {
        if (shared_count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete_obj();
            if (weak_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                ::delete this;
            }
        }
    }

    void decrement_weak() {
        if (weak_count.fetch_sub(1, std::memory_order_acquire) == 1) {
            ::delete this;
        }
    }
};

template <class T, class Deleter = default_delete<T>>
struct control_block : control_block_base {
    [[no_unique_address]] Deleter deleter{};

    control_block() noexcept = default;
    control_block(Deleter d) noexcept : deleter{d} {}
    virtual ~control_block() = default;
};

template <class T, class Deleter = default_delete<T>>
struct control_block_with_ptr : control_block<T, Deleter> {
    T *ptr = nullptr;

    control_block_with_ptr() noexcept = default;
    control_block_with_ptr(T *p) noexcept : ptr{p} {}
    control_block_with_ptr(T *p, Deleter d) noexcept
        : control_block<T, Deleter>(d), ptr{p} {}
    virtual ~control_block_with_ptr() = default;
    auto delete_obj() -> void override { this->deleter(ptr); }
};

template <class T> struct control_block_with_obj : control_block<T> {
    alignas(T) std::array<std::byte, sizeof(T)> storage;

    control_block_with_obj() noexcept = default;
    virtual ~control_block_with_obj() = default;

    auto delete_obj() -> void override {
        std::destroy_at(static_cast<T *>(static_cast<void *>(storage.data())));
    }

    template <class... Args> auto emplace(Args &&...args) -> T * {
        return std::construct_at(
            static_cast<T *>(static_cast<void *>(storage.data())),
            std::forward<Args>(args)...);
    }
};

} // namespace detail

// ****************************************************************************
// *                              shared_ptr                                  *
// ****************************************************************************

template <class T> class shared_ptr {
  public:
    // member types
    using element_type = T;
    using weak_type = weak_ptr<T>;

    // null constructors
    constexpr shared_ptr() noexcept : _ptr{nullptr}, _cb_ptr{nullptr} {}
    constexpr shared_ptr(std::nullptr_t) noexcept
        : _ptr{nullptr}, _cb_ptr{nullptr} {}

    // regular constructors
    template <detail::pointer_convertible_to<T> U>
    explicit shared_ptr(U *ptr)
        : _ptr{ptr}, _cb_ptr{new detail::control_block_with_ptr<U>(ptr)} {
        if constexpr (detail::inherits_from_enable_shared_from_this<T>) {
            // derive from enable_shared_from_this
            _ptr->_weak_this = *this;
        }
    }

    template <detail::pointer_convertible_to<T> U, std::invocable<U *> Deleter>
    shared_ptr(U *ptr, Deleter d)
        : _ptr{ptr}, _cb_ptr{new detail::control_block_with_ptr<U>(ptr, d)} {
        if constexpr (detail::inherits_from_enable_shared_from_this<T>) {
            // derive from enable_shared_from_this
            _ptr->_weak_this = *this;
        }
    }

    // copy/move constructors
    shared_ptr(const shared_ptr &other) noexcept
        : _ptr{other._ptr}, _cb_ptr{other._cb_ptr} {
        if (_ptr != nullptr)
            _cb_ptr->shared_count.fetch_add(1, std::memory_order_relaxed);
    }
    shared_ptr(shared_ptr &&other) noexcept
        : _ptr{std::exchange(other._ptr, nullptr)}, _cb_ptr{std::exchange(
                                                        other._cb_ptr,
                                                        nullptr)} {}

    template <detail::pointer_convertible_to<T> U>
    shared_ptr(const shared_ptr<U> &other) noexcept
        : _ptr{other._ptr}, _cb_ptr{other._cb_ptr} {
        if (_ptr != nullptr)
            _cb_ptr->shared_count.fetch_add(1, std::memory_order_relaxed);
    }

    template <detail::pointer_convertible_to<T> U>
    shared_ptr(shared_ptr<U> &&other) noexcept
        : _ptr{std::exchange(other._ptr, nullptr)}, _cb_ptr{std::exchange(
                                                        other._cb_ptr,
                                                        nullptr)} {}

    // constructed from other smart pointers
    template <detail::pointer_convertible_to<T> U>
    shared_ptr(const weak_ptr<U> &r) : _ptr{nullptr}, _cb_ptr{nullptr} {
        shared_ptr(r.lock()).swap(*this);
        if (_cb_ptr == nullptr)
            throw bad_weak_ptr{};
    }

    template <detail::pointer_convertible_to<T> U, class Deleter>
    shared_ptr(unique_ptr<U, Deleter> &&r) {
        if (r) {
            if constexpr (std::is_reference_v<Deleter>) {
                _cb_ptr = ::new detail::control_block_with_ptr<U>(
                    r.get(), r.get_deleter());
            } else {
                _cb_ptr = ::new detail::control_block_with_ptr<U>(
                    r.get(), std::move(r.get_deleter()));
            }
            _ptr = r.release();

            if constexpr (detail::inherits_from_enable_shared_from_this<T>) {
                // derive from enable_shared_from_this
                _ptr->_weak_this = *this;
            }
        } else {
            _ptr = nullptr;
            _cb_ptr = nullptr;
        }
    }

    // aliasing constructors
    template <class U>
    shared_ptr(const shared_ptr<U> &r, element_type *ptr) noexcept
        : _ptr{ptr}, _cb_ptr{r._cb_ptr} {
        if (r)
            _cb_ptr->shared_count.fetch_add(1, std::memory_order_relaxed);
    }

    template <class U>
    shared_ptr(shared_ptr<U> &&r, element_type *ptr) noexcept
        : _ptr{ptr}, _cb_ptr{std::exchange(r._cb_ptr, nullptr)} {
        r._ptr = nullptr;
    }

    // destructor
    ~shared_ptr() {
        if (_cb_ptr != nullptr)
            _cb_ptr->decrement_shared();
    }

    // assignments
    // copy assignment
    auto operator=(const shared_ptr &rhs) noexcept -> shared_ptr & {
        if (this == &rhs)
            return *this;
        reset();
        _ptr = rhs._ptr;
        _cb_ptr = rhs._cb_ptr;
        if (_cb_ptr != nullptr)
            _cb_ptr->shared_count.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }

    template <detail::pointer_convertible_to<T> U>
    auto operator=(const shared_ptr<U> &rhs) noexcept -> shared_ptr & {
        if (_cb_ptr == rhs._cb_ptr) {
            // if points to the same control block,
            //  do not need to change reference counts
            _ptr = rhs._ptr;
            return *this;
        }
        reset();
        _ptr = rhs._ptr;
        _cb_ptr = rhs._cb_ptr;
        if (_cb_ptr != nullptr)
            _cb_ptr->shared_count.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }

    // move assignment
    auto operator=(shared_ptr &&rhs) noexcept -> shared_ptr & {
        if (this == &rhs)
            return *this;
        reset();
        _ptr = std::exchange(rhs._ptr, nullptr);
        _cb_ptr = std::exchange(rhs._cb_ptr, nullptr);
        return *this;
    }

    template <detail::pointer_convertible_to<T> U>
    auto operator=(shared_ptr<U> &&rhs) noexcept -> shared_ptr & {
        if (this == &rhs)
            return *this;
        reset();
        _ptr = std::exchange(rhs._ptr, nullptr);
        _cb_ptr = std::exchange(rhs._cb_ptr, nullptr);
        return *this;
    }

    // assigned from unique_ptr
    template <class U, class Deleter>
    auto operator=(unique_ptr<U, Deleter> &&rhs) -> shared_ptr & {
        reset();
        shared_ptr(std::move(rhs)).swap(*this);
        return *this;
    }

    // modifiers
    auto swap(shared_ptr &r) noexcept -> void {
        std::swap(_ptr, r._ptr);
        std::swap(_cb_ptr, r._cb_ptr);
    }

    auto reset() noexcept -> void {
        if (_cb_ptr != nullptr) {
            _cb_ptr->decrement_shared();
            _ptr = nullptr;
            _cb_ptr = nullptr;
        }
    }

    template <detail::pointer_convertible_to<T> U> auto reset(U *ptr) -> void {
        reset();
        _ptr = ptr;
        _cb_ptr = ::new detail::control_block_with_ptr<U>(ptr);
    }

    template <detail::pointer_convertible_to<T> U, std::invocable<U *> Deleter>
    auto reset(U *ptr, Deleter d) -> void {
        reset();
        _ptr = ptr;
        _cb_ptr = ::new detail::control_block_with_ptr<U>(ptr, d);
    }

    // observers
    auto get() const noexcept -> element_type * { return _ptr; }
    auto operator*() const noexcept(noexcept(*std::declval<element_type *>()))
        -> element_type & {
        return *get();
    }
    auto operator->() const noexcept -> element_type * { return get(); }

    auto use_count() const noexcept -> long {
        return _cb_ptr ? _cb_ptr->shared_count.load(std::memory_order_relaxed)
                       : 0;
    }
    explicit operator bool() const noexcept { return get() != nullptr; }

    // comparison
    template <class U>
    [[nodiscard]] auto operator==(const shared_ptr<U> &rhs) const noexcept
        -> bool {
        return get() == rhs.get();
    }

    [[nodiscard]] auto operator==(std::nullptr_t) const noexcept -> bool {
        return get() == nullptr;
    }

  private:
    element_type *_ptr;
    detail::control_block_base *_cb_ptr;

    template <class U> friend class shared_ptr;
    template <class U> friend class weak_ptr;

    template <class U, class... Args>
    friend auto make_shared(Args &&...args) -> shared_ptr<U>;
};

// deduction guides
template <class T> shared_ptr(weak_ptr<T>) -> shared_ptr<T>;

template <class T, class D> shared_ptr(unique_ptr<T, D>) -> shared_ptr<T>;

template <class U, class... Args>
auto make_shared(Args &&...args) -> shared_ptr<U> {
    shared_ptr<U> sp{};
    auto cb_ptr = ::new detail::control_block_with_obj<U>{};
    sp._ptr = cb_ptr->emplace(std::forward<Args>(args)...);
    sp._cb_ptr = cb_ptr;

    if constexpr (detail::inherits_from_enable_shared_from_this<U>) {
        // derive from enable_shared_from_this
        sp._ptr->_weak_this = sp;
    }
    return sp;
}

// ****************************************************************************
// *                              weak_ptr                                    *
// ****************************************************************************
template <class T> class weak_ptr {
  public:
    using element_type = T;

    // constructors
    constexpr weak_ptr() noexcept : _ptr{nullptr}, _cb_ptr{nullptr} {}

    // copy constructors
    weak_ptr(const weak_ptr &other) noexcept
        : _ptr{other._ptr}, _cb_ptr{other._cb_ptr} {
        if (_cb_ptr)
            _cb_ptr->weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    template <detail::pointer_convertible_to<T> U>
    weak_ptr(const weak_ptr<U> &other) noexcept
        : _ptr{other._ptr}, _cb_ptr{other._cb_ptr} {
        if (_cb_ptr)
            _cb_ptr->weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    template <detail::pointer_convertible_to<T> U>
    weak_ptr(const shared_ptr<U> &other) noexcept
        : _ptr{other._ptr}, _cb_ptr{other._cb_ptr} {
        if (_cb_ptr)
            _cb_ptr->weak_count.fetch_add(1, std::memory_order_relaxed);
    }

    // move constructors
    weak_ptr(weak_ptr &&other) noexcept
        : _ptr{std::exchange(other._ptr, nullptr)}, _cb_ptr{std::exchange(
                                                        other._cb_ptr,
                                                        nullptr)} {}

    template <detail::pointer_convertible_to<T> U>
    weak_ptr(weak_ptr<U> &&other) noexcept
        : _ptr{std::exchange(other._ptr, nullptr)}, _cb_ptr{std::exchange(
                                                        other._cb_ptr,
                                                        nullptr)} {}

    // destructor
    ~weak_ptr() {
        if (_cb_ptr)
            _cb_ptr->decrement_weak();
    }

    // assignments
    auto operator=(weak_ptr rhs) noexcept -> weak_ptr & {
        swap(rhs);
        return *this;
    }

    template <class U> auto operator=(weak_ptr<U> rhs) noexcept -> weak_ptr & {
        weak_ptr(std::move(rhs)).swap(*this);
        return *this;
    }

    template <detail::pointer_convertible_to<T> U>
    auto operator=(const shared_ptr<U> &rhs) noexcept -> weak_ptr & {
        reset();
        _ptr = rhs._ptr;
        _cb_ptr = rhs._cb_ptr;
        if (_cb_ptr)
            _cb_ptr->weak_count.fetch_add(1, std::memory_order_relaxed);
        return *this;
    }

    // modifiers
    auto reset() noexcept -> void {
        if (_cb_ptr) {
            _cb_ptr->decrement_weak();
            _cb_ptr = nullptr;
            _ptr = nullptr;
        }
    }

    auto swap(weak_ptr &other) noexcept -> void {
        std::swap(_ptr, other._ptr);
        std::swap(_cb_ptr, other._cb_ptr);
    }

    // observers
    auto use_count() const noexcept -> long {
        return _cb_ptr ? _cb_ptr->shared_count.load(std::memory_order_relaxed)
                       : 0;
    }

    auto expired() const noexcept -> bool { return use_count() == 0; }

    auto lock() const noexcept -> shared_ptr<T> {
        shared_ptr<T> sp{};
        if (_cb_ptr) {
            // perform lock-free add-if-not-zero operation
            auto count = _cb_ptr->shared_count.load(std::memory_order_relaxed);
            do {
                if (count == 0)
                    return sp;
            } while (_cb_ptr->shared_count.compare_exchange_weak(
                count, count + 1, std::memory_order_relaxed));
            // use std::memory_order_relaxed here because there is nothing to
            // acquire or release

            sp._ptr = _ptr;
            sp._cb_ptr = _cb_ptr;
        }

        return sp;
    }

  private:
    element_type *_ptr;
    detail::control_block_base *_cb_ptr;

    template <class U> friend class weak_ptr;
};

// deduction guide for weak_ptr
template <class T> weak_ptr(shared_ptr<T>) -> weak_ptr<T>;

// ****************************************************************************
// *                      enable_shared_from_this                             *
// ****************************************************************************

template <class T> class enable_shared_from_this {
  protected:
    // constructors
    constexpr enable_shared_from_this() noexcept = default;

    // no move ctor, and ctor simply value-init weak_this
    enable_shared_from_this(const enable_shared_from_this &) noexcept {}

    // destructor
    ~enable_shared_from_this() = default;

    // assignment is noop
    auto operator=(const enable_shared_from_this &) noexcept
        -> enable_shared_from_this & {
        return *this;
    }

  public:
    auto shared_from_this() -> shared_ptr<T> {
        return shared_ptr<T>(_weak_this);
    }

    auto shared_from_this() const -> shared_ptr<T const> {
        return shared_ptr<T const>(_weak_this);
    }

    auto weak_from_this() noexcept -> weak_ptr<T> {
        return weak_ptr<T>(_weak_this);
    }

    auto weak_from_this() const noexcept -> weak_ptr<T const> {
        return weak_ptr<T const>(_weak_this);
    }

  private:
    weak_ptr<T> _weak_this{};

    friend class shared_ptr<T>;

    template <class U, class... Args>
    friend auto make_shared(Args &&...args) -> shared_ptr<U>;
};

} // namespace mystd