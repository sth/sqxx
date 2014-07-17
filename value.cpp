
// (c) 2013 Stephan Hohe

#include "value.hpp"
#include <sqlite3.h>

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
std::string value::val<std::string>() const {
	const unsigned char *text = sqlite3_value_text(handle);
	int bytes = sqlite3_value_bytes(handle);
	return std::string(reinterpret_cast<const char*>(text), bytes);
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

} // namespace sqxx

