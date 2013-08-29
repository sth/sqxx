
// (c) 2013 Stephan Hohe

#include "parameter.hpp"
#include "sqxx.hpp"

#include <sqlite3.h>

namespace sqxx {

parameter::parameter(statement &a_stmt, int a_idx) : stmt(a_stmt), idx(a_idx) {
}

const char* parameter::name() const {
	const char *n = sqlite3_bind_parameter_name(stmt.raw(), idx+1);
	if (!n)
		throw error(SQLITE_RANGE, "cannot determine parameter name");
	return n;
}

} // namespace sqxx

