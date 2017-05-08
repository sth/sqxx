
// (c) 2013 Stephan Hohe

#if !defined(SQXX_FUNCTION_HPP_INCLUDED)
#define SQXX_FUNCTION_HPP_INCLUDED

#include "connection.hpp"
#include "value.hpp"
#include "context.hpp"
#include "callable/callable.hpp"
#include <functional>
#include <memory>

// Function types with C calling convention for callbacks
// https://stackoverflow.com/a/5590050/
extern "C" typedef void sqxx_function_call_type(sqlite3_context*, int, sqlite3_value**);
extern "C" typedef void sqxx_function_destroy_type(void*);

extern "C" void* sqlite3_user_data(sqlite3_context*);

namespace sqxx {

namespace detail {

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
	static R apply(Fun f, sqlite3_value** argv, Values... args) {
		unused(argv);
		return f(std::forward<Values>(args)...);
	}
};

template<int N, typename R, typename Fun>
R apply_value_array(Fun f, sqlite3_value **argv) {
	return apply_value_array_st<0, N, R, Fun>::apply(std::forward<Fun>(f), argv);
}


void function_register(sqlite3 *handle, const char *name, int argc,
		void *data, sqxx_function_call_type *fun, sqxx_function_destroy_type *destroy);


template<typename Callable>
sqxx_function_destroy_type function_destroy_object;

template<typename Callable>
void function_destroy_object(void *data) {
	delete reinterpret_cast<Callable*>(data);
}



// Create a SQL function from a statically known function pointer

template<typename Function, Function *Fun>
sqxx_function_call_type function_call_staticfptr;

template<typename Function, Function *Fun>
void function_call_staticfptr(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;
	context ctx(handle);
	try {
		if (argc != traits::argc) {
			ctx.result_error_misuse();
			return;
		}
		ctx.result<typename traits::return_type>(
				apply_value_array<traits::argc, typename traits::return_type>(Fun, argv)
			);
	}
	catch (const error &e) {
		ctx.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		ctx.result_error_nomem();
	}
	catch (const std::exception &e) {
		ctx.result_error(e.what());
	}
	catch (...) {
		ctx.result_error_misuse();
	}
}


// Create a SQL function from a callable object
template<typename Callable>
sqxx_function_call_type function_call_ptr;

template<typename Callable>
void function_call_ptr(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	typedef callable_traits<Callable> traits;
	context ctx(handle);
	Callable *callable = reinterpret_cast<Callable*>(sqlite3_user_data(handle));
	try {
		if (argc != traits::argc) {
			ctx.result_error_misuse();
			return;
		}
		ctx.result<typename traits::return_type>(
				apply_value_array<traits::argc, typename traits::return_type>(*callable, argv)
			);
	}
	catch (const error &e) {
		ctx.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		ctx.result_error_nomem();
	}
	catch (const std::exception &e) {
		ctx.result_error(e.what());
	}
	catch (...) {
		ctx.result_error_misuse();
	}
}


// Data associated with an aggregate

struct aggregate_data {
	virtual ~aggregate_data() {}
	virtual void step_call(context &ctx, int nargs, sqlite3_value **values) = 0;
	virtual void final_call(context &ctx) = 0;

protected:
	// forwarding for friend access to context::aggregate_context()
	void *get_aggregate_context(context &ctx, int bytes) {
		return ctx.aggregate_context(bytes);
	}
};

template<typename S>
struct aggregate_invocation_data {
	S state;
	bool initialized;
	aggregate_invocation_data(const S &zero) : state(zero), initialized(true) {
	}
};

// A std::unique_ptr<> variant that takes ownership of the pointed-to object,
// but not the pointed-to memory. When the `ownedobj_ptr<>` goes out of scope,
// if calls the pointed-to object's destructor, but doesn't free the
// associated memory.
//
// This is useful when an object is stored in memory that is managed by sqlite.
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
	const S state_zero;
	typedef std::function<void (S&, Ps...)> stepfun_t;
	stepfun_t stepfun;
	typedef std::function<R (const S&)> finalfun_t;
	finalfun_t finalfun;

	typedef aggregate_invocation_data<S> invocation_data;

public:
	aggregate_data_t(const S &state_zero_arg, const stepfun_t &stepfun_arg, const finalfun_t &finalfun_arg)
		: state_zero(state_zero_arg), stepfun(stepfun_arg), finalfun(finalfun_arg) {
	}

private:
	// Returns the aggregation state, creating it if it doesn't exist yet.
	invocation_data* get_state_create(context &ctx) {
		invocation_data *as = reinterpret_cast<invocation_data*>(
				this->get_aggregate_context(ctx, sizeof(invocation_data))
			);
		if (!as) {
			throw std::bad_alloc();
		}
		// Sqlite zeros freshly allocated memory, so `as->initialized` will
		// start out as `false` if we haven't called the constructor yet.
		if (!as->initialized) {
			// Construct object, with a state copied from `state_zero`
			new(as) invocation_data(state_zero);
		}
		return as;
	}

	// Returns the aggregation state, if there is any
	invocation_data* get_state_existing(context &ctx) {
		// Calling with size 0. We'll get any existing data, or a nullptr if
		// there isn't any.
		invocation_data *as = reinterpret_cast<invocation_data*>(
				this->get_aggregate_context(ctx, 0)
			);
		return as;
	}

public:
	void step_call(context &ctx, int nargs, sqlite3_value **values) override {
		if (nargs != NArgs) {
			ctx.result_error_misuse();
			return;
		}
		invocation_data *as = get_state_create(ctx);
		apply_value_array<NArgs, void>([this,&as](Ps... ps){ stepfun(as->state, std::forward<Ps>(ps)...); }, values);
		//std::vector<value> vs(values, values+nargs);
	}
	void final_call(context &ctx) override {
		// Take ownership of state (if any), deleting it on return
		ownedobj_ptr<invocation_data> as(get_state_existing(ctx));
		if (as) {
			// We have state
			ctx.result(finalfun(as->state));
		}
		else {
			// There was no allocated state
			ctx.result(finalfun(state_zero));
		}
	}
};

} // namespace detail


// Implementation of `connection` member functions

// A function pointer passed as a function argument
template<typename Function>
std::enable_if_t<
	std::is_function<std::remove_pointer_t<std::decay_t<Function>>>::value
	>
connection::create_function(const char *name, Function fun) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;

	// We store the function pointer as sqlite user data.
	// There is no special cleanup necessary for this user data.
	FunctionType *fptr = fun;
	detail::function_register(handle, name, traits::argc,
			reinterpret_cast<void*>(fptr), detail::function_call_ptr<FunctionType>, nullptr);
}

template<typename Callable>
std::enable_if_t<
	!std::is_function<std::remove_pointer_t<std::decay_t<Callable>>>::value
	>
connection::create_function(const char *name, Callable callable) {
	typedef std::decay_t<Callable> CallableType;
	typedef callable_traits<CallableType> traits;

	// We store a copy of the callable as sqlite user data.
	// We register a "destructor" that deletes this copy when the function
	// gets removed.
	CallableType *cptr = new CallableType(callable);
	detail::function_register(handle, name, traits::argc,
			reinterpret_cast<void*>(cptr), detail::function_call_ptr<CallableType>, detail::function_destroy_object<CallableType>);
}

// A function specified as a template parameter
template<typename Function, Function *Fun>
std::enable_if_t<
	std::is_function<Function>::value
	>
connection::create_function(const char *name) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;

	// Function type is known statically and we don't need to store any
	// special sqlite user data.
	detail::function_register(handle, name, traits::argc,
			nullptr, detail::function_call_staticfptr<Function, Fun>, nullptr);
}


template<typename State, typename Result, typename... Args>
void connection::create_aggregate(const char *name,
		State zero,
		const std::function<void (State&, Args...)> &stepfun,
		const std::function<Result (const State &)> &finalfun) {
	static const size_t NArgs = parameter_pack_traits<Args...>::count;
	create_aggregate_p(name, NArgs, new detail::aggregate_data_t<NArgs, State, Result, Args...>(zero, stepfun, finalfun));
}

template<typename State, typename StepCallable, typename FinalCallable>
void connection::create_aggregate(const char *name,
		State zero,
		StepCallable stepfun,
		FinalCallable finalfun) {
	create_aggregate(name, std::forward<State>(zero),
			to_stdfunction(std::forward<StepCallable>(stepfun)),
			to_stdfunction(std::forward<FinalCallable>(finalfun)));
}

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

