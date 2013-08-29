
// (c) 2013 Stephan Hohe

#if !defined(SQXX_COLUMN_HPP_INCLUDED)
#define SQXX_COLUMN_HPP_INCLUDED

#include "datatypes.hpp"
#include "statement.hpp"

namespace sqxx {

/** A column of a sql query result */

class column {
public:
	statement &stmt;
	const int idx;

	column(statement &a_stmt, int a_idx);

	/** sqlite3_column_name() */
	const char* name() const;
	/** sqlite3_column_database_name() */
	const char* database_name() const;
	/** sqlite3_column_table_name() */
	const char* table_name() const;
	/** sqlite3_column_origin_name() */
	const char* origin_name() const;

	/** sqlite3_column_type() */
	int type() const;
	/** sqlite3_column_decltype() */
	const char* decl_type() const;

	/** Access the value of the column in the current result row. */
	template<typename T>
	if_sqxx_db_type<T, T> val() const;
};

template<typename T>
if_sqxx_db_type<T, T> column::val() const {
	return stmt.val<T>(idx);
}

} // namespace sqxx

#endif // SQXX_COLUMN_HPP_INCLUDED

