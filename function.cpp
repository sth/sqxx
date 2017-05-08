
// (c) 2013, 2014 Stephan Hohe

#include "function.hpp"
#include "sqxx.hpp"
#include "value.hpp"
#include "error.hpp"
#include <string>
#include <functional>
#include <sqlite3.h>


namespace sqxx {

namespace detail {

void function_register(sqlite3 *handle, const char *name, int argc,
		void *data, sqxx_function_call_type *fun, sqxx_function_destroy_type *destroy) {
	int rv = sqlite3_create_function_v2(handle, name, argc, SQLITE_UTF8, data,
			fun, nullptr, nullptr, destroy);
	if (rv != SQLITE_OK) {
		// Registered destructor is called automatically, no need to delete |adat| ourselves
		throw static_error(rv);
	}
}

} // details

extern "C"
void sqxx_call_aggregate_step(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	detail::aggregate_data *data = reinterpret_cast<detail::aggregate_data*>(sqlite3_user_data(handle));
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
		ctx.result_error_code(SQLITE_MISUSE);
	}
}

extern "C"
void sqxx_call_aggregate_final(sqlite3_context *handle) {
	detail::aggregate_data *data = reinterpret_cast<detail::aggregate_data*>(sqlite3_user_data(handle));
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
		ctx.result_error_code(SQLITE_MISUSE);
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
	// TODO: check error semantic of C function
	if (rv != SQLITE_OK) {
		// Registered destructor is called automatically, no need to delete |adat| ourselves
		throw static_error(rv);
	}
}

} // namepace sqxx

// Types of functions
//
// int f(Param...)
// void f(context, Param...);
// void f(context, std::vector<value>);
