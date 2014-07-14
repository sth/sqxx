
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
	static R apply(Fun f, sqlite3_value** /*argv*/, Values... args) {
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

template<typename S>
struct aggregate_state {
	S state;
	bool initialized;
	aggregate_state(const S &zero) : state(zero), initialized(true) {
	}
};

// A smart pointer that takes ownership of the pointed-to object, but not the
// pointed-to memory. When the `ownedobj_ptr<>` goes out of scope, if calls
// the pointed-to object's destructor, but doesn't free the associated memory.
//
// This is useful when an object is stored in memory that is managed by sqlite.
//
// TODO: Use std::unique_ptr<> with custom deleter?
/*
template<typename T>
struct ownedobj_ptr {
	T *ptr;
	ownedobj_ptr(T *ptr_arg) : ptr(ptr_arg) {
	}
	~ownedobj_ptr() {
		if (ptr) {
			ptr->~aggregate_state();
		}
	}
	T& operator*() {
		return *ptr;
	}
	T* operator->() {
		return ptr;
	}
	operator bool() {
		return ptr;
	}
};
*/

template<typename T>
struct destruct_only {
	void operator()(T *ptr) {
		if (ptr) {
			ptr->~T();
		}
	}
};

template<typename T>
using ownedobj_ptr = std::unique_ptr<T, destruct_only<T>>;

template<size_t NArgs, typename S, typename R, typename... Ps>
class aggregate_data_t : public aggregate_data {
private:
	typedef std::function<void (S&, Ps...)> stepfun_t;
	stepfun_t stepfun;
	typedef std::function<R (const S&)> finalfun_t;
	finalfun_t finalfun;
	const S state_zero;

public:
	aggregate_data_t(const stepfun_t &stepfun_arg, const finalfun_t &finalfun_arg, const S &state_zero_arg)
		: stepfun(stepfun_arg), finalfun(finalfun_arg), state_zero(state_zero_arg) {
	}

private:
	// Returns the aggregation state, creating it if it doesn't exist yet.
	aggregate_state<S>* get_state_create(sqlite3_context *handle) {
		aggregate_state<S> *as = reinterpret_cast<aggregate_state<S>*>(
				sqlite3_aggregate_context(handle, sizeof(aggregate_state<S>))
			);
		if (!as) {
			throw std::bad_alloc();
		}
		// Sqlite zeros freshly allocated memory, so `as->initialized` will
		// start out as `false` if we haven't called the constructor yet.
		if (!as->initialized) {
			// Construct object, with a state copied from `state_zero`
			new(as) aggregate_state<S>(state_zero);
		}
		return as;
	}

	// Returns the aggregation state, if there is any
	aggregate_state<S>* get_state_existing(sqlite3_context *handle) {
		aggregate_state<S> *as = reinterpret_cast<aggregate_state<S>*>(
				sqlite3_aggregate_context(handle, 0)
			);
		return as;
	}

public:
	void step_call(context &ctx, int nargs, sqlite3_value **values) override {
		if (nargs != NArgs) {
			ctx.result_error_code(SQLITE_MISUSE);
			return;
		}
		aggregate_state<S> *as = get_state_create(ctx.raw());
		apply_value_array<NArgs, void>([this,&as](Ps... ps){ stepfun(as->state, std::forward<Ps>(ps)...); }, values);
		//std::vector<value> vs(values, values+nargs);
	}
	void final_call(context &ctx) override {
		// Take ownership of state, deleting it on return
		ownedobj_ptr<aggregate_state<S>> as(get_state_existing(ctx.raw()));
		//std::unique_ptr<aggregate_state<S>, destruct_only<aggregate_state<S>>> as(get_state_existing(ctx.raw()));
		if (as) {
			ctx.result(finalfun(as->state));
		}
		else {
			ctx.result(finalfun(state_zero));
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
void connection::create_aggregate_reduce(const char *name, const std::function<Result (Result, Args...)> &aggregator, Result zero) {
	// TODO: does [=aggregator](){ ... } capture aggregator by-copy?
	const std::function<Result (Result, Args...)> a(aggregator);
	create_aggregate(name,
			[a](Result &state, Args... args) { state = a(std::forward<Result&>(state), std::forward<Args...>(args...)); },
			[](const Result &state) -> Result { return state; },
			zero
		);
}

template<typename Result, typename Callable>
void connection::create_aggregate_reduce(const char *name, Callable aggregator, Result zero) {
	create_aggregate_reduce(name, to_stdfunction(std::forward<Callable>(aggregator)), zero);
}

} // namespace sqxx

#endif // SQXX_FUNC_HPP_INCLUDED

