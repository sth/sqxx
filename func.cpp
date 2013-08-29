
// (c) 2013 Stephan Hohe

#include "func.hpp"
#include "sqxx.hpp"
#include "detail.hpp"
#include <string>
#include <functional>
#include <sqlite3.h>
#include "detail.hpp"


namespace sqxx {

value::value(sqlite3_value *handle_arg) : handle(handle_arg) {
}

bool value::null() const {
	return (sqlite3_value_type(handle) == SQLITE_NULL);
}

template<>
int value::val<int>() const {
	return sqlite3_value_int(handle);
}

template<>
int64_t value::val<int64_t>() const {
	return sqlite3_value_int64(handle);
}

template<>
double value::val<double>() const {
	return sqlite3_value_double(handle);
}

template<>
const char* value::val<const char*>() const {
	const unsigned char *text = sqlite3_value_text(handle);
	return reinterpret_cast<const char*>(text);
}

template<>
blob value::val<blob>() const {
	// Correct order to call functions according to http://www.sqlite.org/c3ref/column_blob.html
	const void *data = sqlite3_value_blob(handle);
	int len = sqlite3_value_bytes(handle);
	return blob(data, len);
}

value::operator int() const { return val<int>(); }
value::operator int64_t() const { return val<int64_t>(); }
value::operator double() const { return val<double>(); }
value::operator const char*() const { return val<const char*>(); }
value::operator blob() const { return val<blob>(); }

void context::result_misuse() {
	result_error_code(SQLITE_MISUSE);
}

void context::result_error_code(int code) {
	sqlite3_result_error_code(handle, code);
}

template<>
void context::result<int>(int value) {
	sqlite3_result_int(handle, value);
}

sqlite3_context* context::raw() {
	return handle;
}

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

} // namepace sqxx

