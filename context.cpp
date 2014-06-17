
// (c) 2013 Stephan Hohe

#include "context.hpp"
#include <sqlite3.h>

namespace sqxx {

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

} // namespace sqxx

