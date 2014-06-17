
// (c) 2013 Stephan Hohe

#if !defined(SQXX_FUNC_HPP_INCLUDED)
#define SQXX_FUNC_HPP_INCLUDED

#include "datatypes.hpp"
#include "value.hpp"
#include "context.hpp"
#include "sqxx.hpp"
#include <functional>
#include <sqlite3.h>

namespace sqxx {

namespace detail {

// count number of types

template<typename... Ts>
struct type_count;

template<>
struct type_count<> {
	static const size_t value = 0;
};

template<typename T1, typename... Ts>
struct type_count<T1, Ts...> {
	static const size_t value = 1 + type_count<Ts...>::value;
};


// Apply an array of sqlite3_value's to a function

template<int I, int N, typename R, typename Fun>
struct apply_value_array_st {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value **argv, Values... args) {
		return apply_value_array_st<I+1, N, R, Fun>::template apply<Values..., value>(
				std::forward<Fun>(f),
				argv,
				std::forward<Values>(args)..., 
				value(argv[I])
			);
	}
};

template<int I, typename R, typename Fun>
struct apply_value_array_st<I, I, R, Fun> {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value **argv, Values... args) {
		return f(std::forward<Values>(args)...);
	}
};

template<int N, typename R, typename Fun>
R apply_value_array(Fun f, sqlite3_value **argv) {
	return apply_value_array_st<0, N, R, Fun>::apply(f, argv);
}

struct function_data {
	virtual ~function_data() {
	}
	virtual void call(context &ctx, int argc, sqlite3_value **argv) = 0;
};

template<size_t NArgs, typename R, typename... Ps>
struct function_data_t : function_data {
	std::function<R (Ps...)> fun;

	function_data_t(const std::function<R (Ps...)> &fun_arg) : fun(fun_arg) {
	}

	virtual void call(context &ctx, int argc, sqlite3_value **argv) override {
		if (argc != NArgs) {
			ctx.result_misuse();
		}
		else {
			ctx.result<R>(apply_value_array<NArgs, R>(fun, argv));
		}
	}
};

template<size_t NArgs, typename Fun, typename R, typename... Ps>
struct function_data_gen_t : function_data {
	Fun fun;

	function_data_gen_t(Fun fun_arg) : fun(std::forward<Fun>(fun_arg)) {
	}

	virtual void call(context &ctx, int argc, sqlite3_value **argv) override {
		if (argc != NArgs) {
			ctx.result_misuse();
		}
		else {
			ctx.result<R>(apply_value_array<NArgs, R>(fun, argv));
		}
	}
};


} // namespace detail

template<typename R, typename... Ps>
void connection::create_function(const char *name, const std::function<R (Ps...)> &fun) {
	static const size_t NArgs = detail::type_count<Ps...>::value;
	create_function_p(name, NArgs, new detail::function_data_t<NArgs, R, Ps...>(fun));
}

} // namespace sqxx

#endif // SQXX_FUNC_HPP_INCLUDED

