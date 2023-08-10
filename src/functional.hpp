#pragma once

#include <concepts>
#include <exception>
#include <memory>
#include <type_traits>
#include <utility>

namespace mystl {

namespace detail {

template <class CallablePtr, class R, class... Args>
inline auto do_call(void *callable_ptr, Args... args) -> R {
    return (*reinterpret_cast<CallablePtr>(callable_ptr))(
        std::forward<Args>(args)...);
}

// helper concept and type traits for deduction guides
template <class F>
concept no_overload_callable = requires { &F::operator(); };

template <class> struct memfn_sig;

template <class R, class G, class... Args> struct memfn_sig<R (G::*)(Args...)> {
    using type = R(Args...);
};

template <class R, class G, class... Args>
struct memfn_sig<R (G::*)(Args...) const> {
    using type = R(Args...);
};

template <class T> using memfn_sig_t = memfn_sig<T>::type;

} // namespace detail

class bad_function_call : public std::exception {
  public:
    virtual auto what() const noexcept -> const char * {
        return "bad function call";
    }
};

// ****************************************************************************
// *                                 function_ref                             *
// ****************************************************************************

template <class>
class function_ref; // undefined if template argument is not function signature

template <class R, class... Args> class function_ref<R(Args...)> {
    using func_ptr_t = R (*)(Args...);
    using do_call_t = R (*)(void *, Args...);

  public:
    using result_type = R;

    // constructors
    function_ref() noexcept : callable_ptr(nullptr), do_call_ptr(nullptr) {}

    function_ref(func_ptr_t f_ptr) noexcept
        : callable_ptr(reinterpret_cast<void *>(f_ptr)),
          do_call_ptr(&detail::do_call<func_ptr_t, R, Args...>) {}

    template <class F>
        requires std::invocable<F, Args...> &&
                     std::same_as<R, std::invoke_result_t<F, Args...>> &&
                     (!std::same_as<std::remove_const_t<F>,
                                    function_ref<R(Args...)>>)
    function_ref(F &f) noexcept
        : callable_ptr(&f), do_call_ptr(&detail::do_call<F *, R, Args...>) {}

    auto operator()(Args... args) const -> R {
        if (callable_ptr == nullptr)
            throw bad_function_call{};
        return do_call_ptr(callable_ptr, std::forward<Args>(args)...);
    }

  private:
    void *callable_ptr;
    do_call_t do_call_ptr;
};

// deduction guides for std::function_ref
template <class R, class... Args>
function_ref(R (*)(Args...)) -> function_ref<R(Args...)>;

template <detail::no_overload_callable F>
function_ref(F) -> function_ref<detail::memfn_sig_t<decltype(&F::operator())>>;

// ****************************************************************************
// *                            reference_wrapper                             *
// ****************************************************************************

template <class T>
    requires(!std::is_reference_v<T>)
class reference_wrapper {
  public:
    using type = T;
    constexpr reference_wrapper(T &t) noexcept : ptr(std::addressof(t)) {}
    constexpr operator T &() const noexcept { return *ptr; }
    constexpr auto get() const noexcept -> T & { return *ptr; }

  private:
    T *ptr;
};

template <class T> reference_wrapper(T &) -> reference_wrapper<T>;

} // namespace mystl