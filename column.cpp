
// (c) 2013 Stephan Hohe

#include "column.hpp"
#include "sqxx.hpp"
#include <sqlite3.h>

namespace sqxx {

column::column(statement &a_stmt, int a_idx) : stmt(a_stmt), idx(a_idx) {
}

const char* column::name() const {
	return sqlite3_column_name(stmt.raw(), idx);
}

#if defined(SQLITE_ENABLE_COLUMN_METADATA)

const char* column::database_name() const {
	return sqlite3_column_database_name(stmt.raw(), idx);
}

const char* column::table_name() const {
	return sqlite3_column_table_name(stmt.raw(), idx);
}

const char* column::origin_name() const {
	return sqlite3_column_origin_name(stmt.raw(), idx);
}

#endif

int column::type() const {
	return sqlite3_column_type(stmt.raw(), idx);
}

const char* column::decl_type() const {
	return sqlite3_column_decltype(stmt.raw(), idx);
}

} // namespace sqxx

