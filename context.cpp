
// (c) 2013, 2014 Stephan Hohe

#include "context.hpp"
#include <sqlite3.h>

namespace sqxx {

void context::result_error_misuse() {
	result_error_code(SQLITE_MISUSE);
}

void context::result_error(const char *msg) {
	sqlite3_result_error(handle, msg, -1);
}

void context::result_error_code(int code) {
	sqlite3_result_error_code(handle, code);
}

void context::result_error_nomem() {
	sqlite3_result_error_nomem(handle);
}

void context::result_null() {
	result();
}

void context::result() {
	sqlite3_result_null(handle);
}

template<>
void context::result<int>(int value) {
	sqlite3_result_int(handle, value);
}

template<>
void context::result<int64_t>(int64_t value) {
	sqlite3_result_int64(handle, value);
}

template<>
void context::result<double>(double value) {
	sqlite3_result_double(handle, value);
}

template<>
void context::result(const char *value, bool copy) {
	if (value) {
		sqlite3_result_text(handle, value, -1, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	}
	else {
		sqlite3_result_null(handle);
	}
}



template<>
void context::result(const std::string &value, bool copy) {
#if SQLITE_VERSION_NUMBER >= 3008007
	sqlite3_result_text64(handle, value.c_str(), value.length(), (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
#else
	sqlite3_result_text(handle, value.c_str(), value.length(), (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
#endif
}

template<>
void context::result(const blob &value, bool copy) {
	if (value.data) {
#if SQLITE_VERSION_NUMBER >= 3008007
		sqlite3_result_blob64(handle, value.data, value.length, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
#else
		sqlite3_result_blob(handle, value.data, value.length, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
#endif
	}
	else {
#if SQLITE_VERSION_NUMBER >= 3008011
		sqlite3_result_zeroblob64(handle, value.length);
#else
		sqlite3_result_zeroblob(handle, value.length);
#endif
	}
}

void* context::aggregate_context(int bytes) {
	return sqlite3_aggregate_context(handle, bytes);
}

sqlite3_context* context::raw() {
	return handle;
}

} // namespace sqxx

