
// Implementation of connection::create_aggregate()

#if !defined(SQXX_CONNECTION_CREATE_AGGREGATE_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_CREATE_AGGREGATE_IMPL_HPP_INCLUDED

namespace sqxx {
namespace detail {

// Data associated with an aggregate

template<typename State>
struct aggregate_invocation_data {
	bool initialized;
	State state;
	aggregate_invocation_data(const State &zero) : initialized(true), state(zero) {
	}
};

template<typename StateD, typename StepD, typename FinalD>
class aggregate_data {
private:
	const StateD state_zero;
	StepD stepfun;
	FinalD finalfun;

	static constexpr int NArgs = callable_traits<StepD>::argc - 1;

public:
	template<typename State, typename StepFun, typename FinalFun>
	aggregate_data(State &&state_zero_arg, StepFun &&stepfun_arg, FinalFun &&finalfun_arg)
		: state_zero(std::forward<State>(state_zero_arg)), stepfun(std::forward<StepFun>(stepfun_arg)), finalfun(std::forward<FinalFun>(finalfun_arg)) {
	}

private:
	typedef aggregate_invocation_data<StateD> invocation_data;

	// Returns the aggregation state, creating it if it doesn't exist yet.
	invocation_data* get_state_create(context &ctx) {
		invocation_data *as = reinterpret_cast<invocation_data*>(
				ctx.aggregate_context(sizeof(invocation_data))
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
				ctx.aggregate_context(0)
			);
		return as;
	}

public:
	void step_call(context &ctx, int argc, sqlite3_value **values) {
		if (argc != NArgs) {
			ctx.result_error_misuse();
			return;
		}
		invocation_data *as = get_state_create(ctx);
		apply_value_array<NArgs, void>(stepfun, values, as->state);
		//std::vector<value> vs(values, values+nargs);
	}

	void final_call(context &ctx) {
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


template<typename StateD, typename StepD, typename FinalD>
sqxx_function_call_type aggregate_call_step;

template<typename StateD, typename StepD, typename FinalD>
void aggregate_call_step(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	typedef aggregate_data<StateD, StepD, FinalD> data_t;
	data_t *data = reinterpret_cast<data_t*>(sqlite3_user_data(handle));
	context ctx(handle);
	try {
		data->step_call(ctx, argc, argv);
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

template<typename StateD, typename StepD, typename FinalD>
sqxx_function_final_type aggregate_call_final;

template<typename StateD, typename StepD, typename FinalD>
void aggregate_call_final(sqlite3_context *handle) {
	typedef aggregate_data<StateD, StepD, FinalD> data_t;
	data_t *data = reinterpret_cast<data_t*>(sqlite3_user_data(handle));
	context ctx(handle);
	try {
		data->final_call(ctx);
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

void create_aggregate_register(sqlite3 *handle, const char *name, int nargs, void *data,
		sqxx_function_call_type *stepfun, sqxx_function_final_type *finalfun,
		sqxx_function_destroy_type *destroy);

} // namespace detail

template<typename State, typename StepCallable, typename FinalCallable>
void connection::create_aggregate(const char *name,
		State &&zero,
		StepCallable stepfun,
		FinalCallable finalfun) {
	typedef std::decay_t<State> StateD;
	typedef std::decay_t<StepCallable> StepD;
	typedef std::decay_t<FinalCallable> FinalD;
	typedef callable_traits<StepD> traits;
	typedef detail::aggregate_data<StateD, StepD, FinalD> data_t;
	data_t *adat = new data_t(
			std::forward<State>(zero),
			std::forward<StepCallable>(stepfun),
			std::forward<FinalCallable>(finalfun)
		);
	detail::create_aggregate_register(handle, name, traits::argc-1, reinterpret_cast<void*>(adat),
			detail::aggregate_call_step<StateD, StepD, FinalD>,
			detail::aggregate_call_final<StateD, StepD, FinalD>,
			detail::function_destroy_object<data_t>);
}

template<typename State, typename StepCallable, typename FinalCallable>
void connection::create_aggregate(const std::string &name, State &&zero, StepCallable step_fun,
		FinalCallable final_fun) {
	create_aggregate(name.c_str(), std::forward<State>(zero), std::forward<StepCallable>(step_fun),
			std::forward<FinalCallable>(final_fun));
}

template<typename State, typename StepCallable>
void connection::create_aggregate(const char *name, State &&zero, StepCallable step_fun) {
	create_aggregate<State, StepCallable>(name, std::forward<State>(zero), step_fun,
			[](const State &state) -> State { return state; }
		);
}

template<typename State, typename StepCallable>
void connection::create_aggregate(const std::string &name, State &&zero, StepCallable step_fun) {
	create_aggregate(name.c_str(), std::forward<State>(zero), std::forward<StepCallable>(step_fun));
}

} // namespace sqxx

#endif // SQXX_CONNECTION_CREATE_AGGREGATE_IMPL_HPP_INCLUDED

