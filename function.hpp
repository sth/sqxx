
// (c) 2013 Stephan Hohe

#if !defined(SQXX_FUNCTION_HPP_INCLUDED)
#define SQXX_FUNCTION_HPP_INCLUDED

#include "connection.hpp"
#include "value.hpp"
#include "context.hpp"
#include "callable/callable.hpp"
#include <functional>
#include <memory>

namespace sqxx {

// Implementation of `connection` member functions

template<typename Result, typename... Args>
void connection::create_aggregate_reduce(const char *name, Result zero, const std::function<Result (Result, Args...)> &aggregator) {
	const std::function<Result (Result, Args...)> a(aggregator);
	create_aggregate(name, zero,
			[a](Result &state, Args... args) { state = a(std::forward<Result&>(state), std::forward<Args...>(args...)); },
			[](const Result &state) -> Result { return state; }
		);
}

template<typename Result, typename Callable>
void connection::create_aggregate_reduce(const char *name, Result zero, Callable aggregator) {
	create_aggregate_reduce(name, zero, to_stdfunction(std::forward<Callable>(aggregator)));
}

} // namespace sqxx

#endif // SQXX_FUNCTION_HPP_INCLUDED

