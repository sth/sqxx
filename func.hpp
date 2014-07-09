
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

struct aggregate_data {
	virtual ~aggregate_data() {}
	virtual void step_call(context &ctx, int nargs, sqlite3_value **values) = 0;
	virtual void final_call(context &ctx) = 0;
};

/*
template<class Aggregator>
struct aggregate_data_c : aggregate_data {
	const Aggregator zero_aggr;

	aggregate_data_c(const Aggregator &zero_aggr_arg) : zero_aggr(zero_aggr_arg) {
	}

	Aggregator& get_aggr_create(sqlite3_context *handle) {
		Aggregator *aggr = reinterpret_cast<Aggregator*>(
				sqlite3_aggregate_context(handle, sizeof(Aggregator))
			);
		if (!aggr) {
			throw std::bad_alloc();
		}
		new(aggr) Aggregator(zero_aggr);
		return *aggr;
	}
	const Aggregator& get_aggr_existing(sqlite3_context *handle) {
		Aggregator *aggr = reinterpret_cast<Aggregator*>(
				sqlite3_aggregate_context(handle, sizeof(Aggregator))
			);
		if (aggr) 
			return *aggr;
		else
			return zero_aggr;
	}
	void step(sqlite3_context *handle, int nargs, sqlite3_value **values) override {
		static const size_t NArgs = callable_traits<decltype(Aggregator::step)>::nargs - 1;
		apply_array(get_aggr_create(handle)->step, nargs, values);
		//std::vector<value> vs(values, values+nargs);
	}
	void final(sqlite3_context *handle) override {
		// Take ownership of aggr, deleting it on return
		std::unique_ptr<Aggregator> aggr(get_aggr_existing(handle));
		context(handle).result(const_cast<const Aggregator>(*aggr).final());
	}
};
*/

template<typename T>
struct unplace_ptr {
	T *ptr;
	unplace_ptr(T *ptr_arg) : ptr(ptr_arg) {
	}
	~unplace_ptr() {
		if (ptr) {
			ptr->~T();
		}
	}
	T& operator*() {
		return *ptr;
	}
	operator bool() {
		return *ptr;
	}
};
	

template<size_t NArgs, typename S, typename R, typename... Ps>
struct aggregate_data_t : aggregate_data {
	typedef std::function<void (S&, Ps...)> stepfun_t;
	stepfun_t stepfun;
	typedef std::function<R (const S&)> finalfun_t;
	finalfun_t finalfun;
	const S state_zero;

	aggregate_data_t(const stepfun_t &stepfun_arg, const finalfun_t &finalfun_arg, const S &state_zero_arg)
		: stepfun(stepfun_arg), finalfun(finalfun_arg), state_zero(state_zero_arg) {
	}

	S& get_state_create(sqlite3_context *handle) {
		S *state = reinterpret_cast<S*>(
				sqlite3_aggregate_context(handle, sizeof(S))
			);
		if (!state) {
			throw std::bad_alloc();
		}
		new(state) S(state_zero);
		return *state;
	}
	S* get_state_existing(sqlite3_context *handle) {
		return reinterpret_cast<S*>(
				sqlite3_aggregate_context(handle, 0)
			);
	}

	void step_call(context &ctx, int nargs, sqlite3_value **values) override {
		apply_value_array<NArgs, void>([this,&ctx](Ps... ps){ stepfun(get_state_create(ctx.raw()), ps...); }, values);
		//std::vector<value> vs(values, values+nargs);
	}
	void final_call(context &ctx) override {
		// Take ownership of state, deleting it on return
		unplace_ptr<S> state = get_state_existing(ctx.raw());
		if (state) {
			ctx.result<R>(finalfun(*state));
		}
		else {
			ctx.result<R>(finalfun(state_zero));
		}
	}
};


} // namespace detail

template<typename R, typename... Ps>
void connection::create_function(const char *name, const std::function<R (Ps...)> &fun) {
	static const size_t NArgs = detail::type_count<Ps...>::value;
	create_function_p(name, NArgs, new detail::function_data_t<NArgs, R, Ps...>(fun));
}

template<typename State, typename Result, typename... Args>
void connection::create_aggregate(const char *name,
		const std::function<void (State&, Args...)> &stepfun,
		const std::function<Result (const State &)> &finalfun,
		State zero) {
	static const size_t NArgs = detail::type_count<Args...>::value;
	create_aggregate_p(name, NArgs, new detail::aggregate_data_t<NArgs, State, Result, Args...>(stepfun, finalfun, zero));
}

template<typename State, typename StepCallable, typename FinalCallable>
void connection::create_aggregate(const char *name,
		StepCallable stepfun,
		FinalCallable finalfun,
		State zero) {
	create_aggregate(name,
			to_stdfunction(std::forward<StepCallable>(stepfun)),
			to_stdfunction(std::forward<FinalCallable>(finalfun)),
			std::forward<State>(zero));
}

template<typename Result, typename... Args>
void connection::create_aggregate(const char *name, const std::function<Result (Result, Args...)> &aggregator, Result zero) {
	// TODO: does [=aggregator](){ ... } capture aggregator by-copy?
	const std::function<Result (Result, Args...)> a(aggregator);
	create_aggregate(name,
			[a](Result &state, Args... args) { state = a(std::forward<Result&>(state), std::forward<Args...>(args...)); },
			[](const Result &state) -> Result { return state; },
			zero
		);
}

template<typename Result, typename Callable>
void connection::create_aggregate(const char *name, Callable aggregator, Result zero) {
	create_aggregate(name, to_stdfunction(aggregator), zero);
}

} // namespace sqxx

#endif // SQXX_FUNC_HPP_INCLUDED

