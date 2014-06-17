
// (c) 2013 Stephan Hohe

#include "func.hpp"
#include "sqxx.hpp"
#include "value.hpp"
#include "detail.hpp"
#include <string>
#include <functional>
#include <sqlite3.h>

namespace sqxx {

extern "C"
void sqxx_function_call(sqlite3_context *ctx, int argc, sqlite3_value** argv) {
	detail::function_data *data = reinterpret_cast<detail::function_data*>(sqlite3_user_data(ctx));
	context c(ctx);
	try {
		data->call(c, argc, argv);
	}
	catch (const error &e) {
		c.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		sqlite3_result_error_nomem(ctx);
	}
	catch (const std::exception &e) {
		sqlite3_result_error(ctx, e.what(), -1);
	}
	catch (...) {
		sqlite3_result_error_code(ctx, SQLITE_MISUSE);
	}
}

extern "C"
void sqxx_function_destroy(void *data) {
	detail::function_data *d = reinterpret_cast<detail::function_data*>(data);
	delete d;
}

void connection::create_function_p(const char *name, int nargs, detail::function_data *fdat) {
	int rv;
	rv = sqlite3_create_function_v2(handle, name, nargs, SQLITE_UTF8, fdat,
			sqxx_function_call, nullptr, nullptr, sqxx_function_destroy);
	if (rv != SQLITE_OK) {
		delete fdat;
		throw static_error(rv);
	}
}



template<class Aggregate>
void create_aggregation() {
}

struct aggregate_data {
	virtual void step(sqlite3_context *handle, int nargs, sqlite3_value **values) = 0;
	virtual void final(sqlite3_context *handle) = 0;
};


template<class Aggregator>
struct aggregate_data_c : aggregate_data {
	Aggregator* get_aggr(sqlite3_context *handle) {
		Aggregator **aggr = reinterpret_cast<Aggregator**>(
				sqlite3_aggregate_context(handle, sizeof(Aggregator*))
			);
		if (!aggr) {
			throw std::bad_alloc();
		}
		if (!*aggr) {
			*aggr = new Aggregator();
		}
		return *aggr;
	}
	void step(sqlite3_context *handle, int nargs, sqlite3_value **values) override {
		apply_array(get_aggr(handle)->step, nargs, values);
		//std::vector<value> vs(values, values+nargs);
	}
	void final(sqlite3_context *handle) override {
		// Take ownership of aggr, deleting it on return
		std::unique_ptr<Aggregator> *aggr = get_aggr(handle);
		context(handle).result(aggr->final());
	}
};

class aggregation {
	virtual void step(...);
	virtual void final(context);
};

template<class Aggregator>
class aggregate_manager {
	virtual Aggregator* create() {
		return new Aggregator();
	}
	virtual void destroy(Aggregator *aggr) {
		delete aggr;
	}
};

/*
class aggregate_manager {
	virtual void create(Aggregator **aggr) {
	}
	virtual void destroy(Aggregator *aggr) {
	}
};

void f() {
	Data data;

	sq.create_aggregate(


class aggregation_arr {
	virtual void step(const std::vector<value> &values);
	virtual void final(context &);
};
*/

typedef std::function<context&(const std::vector<value>&)> function_arr_t;

} // namepace sqxx

// Types of functions
//
// int f(Param...)
// void f(context, Param...);
// void f(context, std::vector<value>);
