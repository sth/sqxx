
// (c) 2013, 2014 Stephan Hohe

#if !defined(STHCXX_CALLABLE_HPP_INCLUDED)
#define STHCXX_CALLABLE_HPP_INCLUDED

#include <stddef.h>
#include <functional>

#include "parameter_pack.hpp"

namespace detail {

/** Define traits for a function type */
template<typename Fun>
struct callable_traits_fn;

template<typename Ret, typename... Args>
struct callable_traits_fn<Ret (Args...)> {
	typedef Ret function_type(Args...);
	typedef Ret return_type;
	static const size_t argc;

	template<size_t N>
	using argument_type = typename parameter_pack_traits<Args...>::template type<N>;
};

template<typename Ret, typename... Args>
const size_t callable_traits_fn<Ret (Args...)>::argc = parameter_pack_traits<Args...>::count;


/** Define traits for a operator() member function pointer type */
template<typename MemFun>
struct callable_traits_memfnp;

template<typename Class, typename Ret, typename... Args>
struct callable_traits_memfnp<Ret (Class::*)(Args...)> : callable_traits_fn<Ret (Args...)> {
};

template<typename Class, typename Ret, typename... Args>
struct callable_traits_memfnp<Ret (Class::*)(Args...) const> : callable_traits_fn<Ret (Args...)> {
};


// classes with operator()
template<typename Callable>
struct callable_traits_d : detail::callable_traits_memfnp<decltype(&Callable::operator())> {
};

// functions
template<typename Ret, typename... Args>
struct callable_traits_d<Ret (Args...)> : detail::callable_traits_fn<Ret (Args...)> {
};

// function pointers
template<typename Ret, typename... Args>
struct callable_traits_d<Ret (*)(Args...)> : detail::callable_traits_fn<Ret (Args...)> {
};

// std::function
template<typename Ret, typename... Args>
struct callable_traits_d<std::function<Ret (Args...)>> : detail::callable_traits_fn<Ret (Args...)> {
};

} // namespace detail


// Main template

/** Traits for a callable (function/functor/lambda/...) */
template<typename Callable>
struct callable_traits : detail::callable_traits_d<typename std::remove_reference<Callable>::type> {
};

/** Convert a callable to a std::function<> */
template<typename Callable>
std::function<typename callable_traits<Callable>::function_type> to_stdfunction(Callable fun) {
	return std::function<typename callable_traits<Callable>::function_type>(std::forward<Callable>(fun));
}

#endif // STHCXX_CALLABLE_HPP_INCLUDED

