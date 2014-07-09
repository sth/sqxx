
// (c) 2013 Stephan Hohe

#include "func.hpp"
#include "sqxx.hpp"
#include "value.hpp"
#include "error.hpp"
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


extern "C"
void sqxx_call_aggregate_step(sqlite3_context *ctx, int argc, sqlite3_value** argv) {
	detail::aggregate_data *data = reinterpret_cast<detail::aggregate_data*>(sqlite3_user_data(ctx));
	context c(ctx);
	try {
		data->step_call(c, argc, argv);
	}
	catch (const error &e) {
		c.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		c.result_error_nomem();
	}
	catch (const std::exception &e) {
		c.result_error(e.what());
	}
	catch (...) {
		c.result_error_code(SQLITE_MISUSE);
	}
}

extern "C"
void sqxx_call_aggregate_final(sqlite3_context *ctx) {
	detail::aggregate_data *data = reinterpret_cast<detail::aggregate_data*>(sqlite3_user_data(ctx));
	context c(ctx);
	try {
		data->final_call(c);
	}
	catch (const error &e) {
		c.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		c.result_error_nomem();
	}
	catch (const std::exception &e) {
		c.result_error(e.what());
	}
	catch (...) {
		c.result_error_code(SQLITE_MISUSE);
	}
}

extern "C"
void sqxx_call_aggregate_destroy(void *data) {
	detail::aggregate_data *d = reinterpret_cast<detail::aggregate_data*>(data);
	delete d;
}

void connection::create_aggregate_p(const char *name, int nargs, detail::aggregate_data *adat) {
	int rv;
	rv = sqlite3_create_function_v2(handle, name, nargs, SQLITE_UTF8, adat,
			nullptr, sqxx_call_aggregate_step, sqxx_call_aggregate_final, sqxx_call_aggregate_destroy);
	if (rv != SQLITE_OK) {
		delete adat;
		throw static_error(rv);
	}
}

} // namepace sqxx

// Types of functions
//
// int f(Param...)
// void f(context, Param...);
// void f(context, std::vector<value>);
