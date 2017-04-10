
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

	/** Wraps [`sqlite3_column_name()`](http://www.sqlite.org/c3ref/column_name.html) */
	const char* name() const;
	/** Wraps [`sqlite3_column_database_name()`](http://www.sqlite.org/c3ref/column_database_name.html) */
#if defined(SQLITE_ENABLE_COLUMN_METADATA)
	const char* database_name() const;
	/** Wraps [`sqlite3_column_table_name()`](http://www.sqlite.org/c3ref/column_database_name.html) */
	const char* table_name() const;
	/** Wraps [`sqlite3_column_origin_name()`](http://www.sqlite.org/c3ref/column_database_name.html) */
	const char* origin_name() const;
#endif

	// TODO: Better return type
	/** Wraps [`sqlite3_column_type()`](http://www.sqlite.org/c3ref/column_blob.html) */
	int type() const;
	/** Wraps [`sqlite3_column_decltype()`](http://www.sqlite.org/c3ref/column_decltype.html) */
	const char* decl_type() const;

	/**
	 * Access the value of the column in the current result row.
	 *
	 * Same as `statement::val()` without the column parameter.
	 * See there for details.
	 */
	template<typename T>
	if_sqxx_db_type<T, T> val() const;
};

} // namespace sqxx

#include "column.impl.hpp"

#endif // SQXX_COLUMN_HPP_INCLUDED

