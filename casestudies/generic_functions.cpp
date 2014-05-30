
	/** Define a SQL function, automatically deduce the number of arguments */
	template<typename Callable>
	void create_function(const char *name, Callable f);
	/** Define a SQL function with the given number of arguments. Use if automatic deduction fails. */
	template<int NArgs, typename Callable>
	void create_function_n(const char *name, Callable f);
	/** Define a SQL function with variable number of arguments. The Callable will be
	 * called with a single parameter, a std::vector<sqxx::value>.
	 */
	template<typename Callable>
	void create_function_vararg(const char *name, Callable f);

	/*
	// TODO
	template<typename Aggregator>
	void create_aggregare(const char *name);

	template<typename AggregatorFactory>
	void create_aggregare(const char *name, AggregatorFactory f);
	*/


static void create_function_p(sqlite3 *handle, const char *name, int nargs, sqxx_fun_data *fdat) {
	int rv;
	rv = sqlite3_create_function_v2(handle, name, nargs, SQLITE_UTF8, fdat,
			sqxx_fun_call, nullptr, nullptr, sqxx_fun_destroy);
	if (rv != SQLITE_OK) {
		delete fdat;
		throw static_error(rv);
	}
}


template<typename Callable>
void connection::create_function(const char *name, Callable f) {
	static const size_t NArgs = callable_traits<Callable>::argc;
	create_function_n<NArgs, Callable>(name, std::forward(f));
}

template<int NArgs, typename Callable>
void connection::create_function_n(const char *name, Callable f) {
	create_function_p(handle, name, NArgs, new sqxx_fun_data_n<NArgs, Callable>(std::forward(f)));
}


template<typename Callable>
struct sqxx_fun_data_vararg : sqxx_fun_data {
	typename std::remove_reference<Callable>::type fun;
	sqxx_fun_data_vararg(Callable a_fun) : fun(std::forward(a_fun)) {
	}

	virtual value call(int argc, sqlite3_value **argv) {
		std::vector<value> args(argv, argv+argc);
		return fun(args);
	}
};

template<typename Callable>
void connection::create_function_vararg(const char *name, Callable f) {
	create_function_p(handle, name, -1, new sqxx_fun_data_vararg<Callable>(std::forward(f)));
}

/*
struct sqxx_aggr_data {
};

template<class Aggregator>
struct sqxx_aggr_data_cls : sqxx_aggr_data {
private:
	Aggregator** get(sqlite3_context *ctx) {
	}

	void init(Aggregator **ptr) {
		if (*ptr == nullptr)
			*ptr = new Aggregator();
	}

public:
	void step(sqlite3_context *ctx) {
		Aggregator **ptr = (Aggregator**)sqlite3_user_data(ctx, sizeof(Aggregator*));
		ensure(ptr);
		(*ptr)->step();
	}

	void final(sqlite3_cotext *ctx) {
		Aggregator **ptr = (Aggregator**)sqlite3_user_data(ctx, 0);
		Aggregator *data;
		if (ptr)
			data = *ptr;
		ensure(data);
		data->step();
		delete data;
	}
};

template<typename Aggregator>
void connection::create_function_vararg(const char *name) {
	create_function_p(handle, name, -1, new sqxx_fun_data_vararg<Callable>(std::forward(f)));
}

template<typename AggregatorFactory>
void connection::create_function_vararg(const char *name, AggregatorFactory f) {
	create_function_p(handle, name, -1, new sqxx_fun_data_vararg<Callable>(std::forward(f)));
}
*/

