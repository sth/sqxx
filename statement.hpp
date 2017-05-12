
// (c) 2013 Stephan Hohe

#if !defined(SQXX_STATEMENT_HPP_INCLUDED)
#define SQXX_STATEMENT_HPP_INCLUDED

#include "datatypes.hpp"
#include "connection.hpp"
#include <map>

// struct from <sqlite3.h>
struct sqlite3_stmt;

namespace sqxx {

class connection;
class parameter;
class column;

/**
 * A sql statement
 *
 * Includes:
 *
 * - Binding of parameters for prepared statements
 * - Accessing of result rows after executing the statement
 *
 * Wraps the C API struct `sqlite3_stmt` and associated functions.
 *
 */
class statement {
private:
	sqlite3_stmt *handle;

public:
	connection &conn;

protected:
	bool completed;

public:
	/**
	 * Constructs a statement object from a connection object and a C API
	 * statement handle.
	 *
	 * Usually not called directly, use `connection::prepare()` instead.
	 *
	 * @conn_arg The connection object associated with the statement. A reference
	 * is stored and the connection object must stay alive until the constructed
	 * `statement` is destroyed.
	 *
	 * @handle_arg A C API statement handle. The constructed `statement` takes
	 * ownership of the C API handle and will close it on destruction.
	 */
	statement(connection &conn_arg, sqlite3_stmt *handle_arg);
	/*
	 * Destroys the object, closing the managed C API handle, if necessary.
	 */
	~statement();

	/** Copy construction is disabled */
	statement(const statement &) = delete;
	/** Copy assignment is disabled */
	statement& operator=(const statement&) = delete;
	/** Move construction is enabled */
	statement(statement &&) = default;
	/** Move assignment is enabled */
	statement& operator=(statement&&) = default;


	/**
	 * Get count of parameters in prepared statement.
	 *
	 * Wraps [`sqlite3_bind_parameter_count()`](http://www.sqlite.org/c3ref/bind_parameter_count.html).
	 */
	int param_count() const;

	/**
	 * Get index of a named parameter.
	 *
	 * Usually only used internally.
	 *
	 * To bind a value to a named it is unnecessary to look up its
	 * index first, the name can be used directly with `bind(name, value)`.
	 *
	 * When the same parameter is bound many times for many queries it
	 * might be useful as a performance optimization to look the name up
	 * only once, but in this case usually a `parameter` object obtained
	 * by `statement::param(name)` will be used.
	 *
	 * Wraps [`sqlite3_bind_parameter_index()`](http://www.sqlite.org/c3ref/bind_parameter_index.html).
	 */
	int param_index(const char *name) const;
	/** Like <statement::param_index(const char*)> */
	int param_index(const std::string &name) const;

	/**
	 * Get a parameter object, by index or name.
	 *
	 * Most commonly no parameter objects are created but parameters are bound directly
	 * with `statement::bind()`.
	 *
	 * An explicit parameter object can be useful if you need to bind the same
	 * parameter for many, many queries and want to avoid the (small) performance
	 * cost of repeated name lookups. Then you could replace repeated
	 * `stmt.bind("paramname", value);` by a single `auto p = stmt.param("paramname")`,
	 * followed by repeated `p.bind(value)`.
	 *
	 * The same performance can also be achieved without `parameter` objects by using
	 * numeric parameter indexes instead of names.
	 */
	parameter param(int idx);
	parameter param(const char *name);
	parameter param(const std::string &name);

	/**
	 * Bind parameter values in prepared statements
	 *
	 * Parameters values of types `int`, `int64_t`, `double`, `const char*`,
	 * std::string` and `sqxx:blob` are supported. For each supported type an
	 * appropriate specialization of `bind<>()` is provided, calling the
	 * underlying C API function.
	 *
	 * Using templates allows the user to specify the desired type as
	 * `bind<type>(idx, value)`. This can be used to easily disambiguate cases
	 * where the compiler can't automatically deduce the correct type/overload.
	 *
	 * The first parameter of each of these functions is an index or a name
	 * of the parameter, the second parameter the value that should be bound.
	 * Parameter indexes start at zero.
	 *
	 *     bind(0, 123);
	 *     bind("paramname", 345):
	 *
	 * For string and blob parameters an optional third parameter is
	 * available, that specifies if sqlite should make an internal copy of
	 * the passed value. If the parameter is `false` and no copy is created, the
	 * caller has to make sure that the passed value stays alive and isn't
	 * destroyed until the query is completed. Default is to create copies
	 * of the passed parameters.
	 *
	 *     bind("param1", std::string("temporary"), true);
	 *     bind("param2", "persistent", false);
	 *
	 * If the passed value isn't exactly one supported by sqlite3, overload
	 * resolution can be ambiguous, which leads to compiler errors. In this
	 * case manually specify the desired parameter type:
	 *
	 *     bind<int64_t>("param3", 123U);
	 *
	 * When `bind` is called without a value, the parameter is set to `NULL`:
	 *
	 *     bind("param4");
	 *
	 * A `NULL` is bound as well when a passed `const char*` is null, or a
	 * `nullptr` is passed directly:
	 *
	 *     const char *str = nullptr;
	 *     bind("param6", str);
	 *     bind("param7", nullptr);
	 *
	 */

	/**
	 * Set a parameter to NULL.
	 *
	 * Wraps [`sqlite3_bind_null()`](http://www.sqlite.org/c3ref/bind_blob.html).
	 */
	void bind(int idx);
	void bind(const char *name) { bind(param_index(name)); }
	void bind(const std::string &name) { bind(name.c_str()); }

	/**
	 * Set a parameter to a `int`, `int64_t` or `double` value.
	 *
	 * Wraps [`sqlite3_bind_int()`](http://www.sqlite.org/c3ref/bind_blob.html),
	 * [`sqlite3_bind_int64()`](http://www.sqlite.org/c3ref/bind_blob.html).
	 * [`sqlite3_bind_double()`](http://www.sqlite.org/c3ref/bind_blob.html).
	 */
	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(int idx, T value);

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const char *name, T value) { bind<T>(param_index(name), value); }
	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const std::string &name, T value) { bind<T>(name.c_str(), value); }

	/**
	 * Set a parameter to a `const char*` value.
	 *
	 * Wraps [`sqlite3_bind_text()`](http://www.sqlite.org/c3ref/bind_blob.html),
	 */
	template<typename T>
	if_selected_type<T, void, const char*>
	bind(int idx, T value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const char *name, T value, bool copy=true) { bind<T>(param_index(name), value, copy); }
	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const std::string &name, T value, bool copy=true) { bind<T>(name.c_str(), value, copy); }

	/**
	 * Set a parameter to a `std::string` or `blob` value.
	 *
	 * Wraps [`sqlite3_bind_text()`](http://www.sqlite.org/c3ref/bind_blob.html),
	 * Wraps [`sqlite3_bind_blob()`](http://www.sqlite.org/c3ref/bind_blob.html),
	 * Wraps [`sqlite3_bind_zeroblob()`](http://www.sqlite.org/c3ref/bind_blob.html),
	 */
	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(int idx, const T &value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const char *name, const T &value, bool copy=true) { bind<T>(param_index(name), value, copy); }
	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const std::string &name, const T &value, bool copy=true) { bind<T>(name.c_str(), value, copy); }

	/**
	 * Reset all bindings on a prepared statement.
	 *
	 * Wraps [`sqlite3_clear_bindings()`](http://www.sqlite.org/c3ref/clear_bindings.html)
	 */
	void clear_bindings();


	// Result columns

	/**
	 * Number of columns in a result set.
	 *
	 * Wraps [`sqlite3_column_count()`](http://www.sqlite.org/c3ref/column_count.html) 
	 */
	int col_count() const;

private:
	// This is just a cache and doesn't change the object state.
	// It's ok to create/update the cache on const objects, so we
	// make it `mutable`.
	mutable bool col_index_table_built = false;
	mutable std::map<std::string, int> col_index_table;

public:
	/**
	 * Return the index of a column with name `name`.
	 *
	 * If there are multiple columns with the same name, the index
	 * of one of them is returned.
	 *
	 * The first call to this function builds a lookup table that
	 * is used to translate names to indexes.
	 *
	 * Uses a lookup table built from [`sqlite3_column_name()`](http://www.sqlite.org/c3ref/column_name.html) calls
	 */
	int col_index(const char *name) const;
	int col_index(const std::string &name) const;

	/**
	 * Get a column object.
	 *
	 * Usually a column object is not necessary, result data can be accessed
	 * directly using `statement::val<type>(columnindex)`.
	 *
	 * A `column` object can be useful to access additional properties of a result
	 * column, like its declared type and name.
	 */

	///*
	// * When the same columns are accessed by name in a large numbers of column rows,
	// * using a column object might slightly improve performance, since name lookup
	// * is only done once. Many calls to `stmt.val<type>("colname")` would be
	// * replaced by a single call `auto c = stmt.col("colname");` and many calls to
	// * `c.val<type>()`.
	// *
	// * The same performance can also be achieved without `column` objects by using
	// * numeric column indexes instead of names.
	// */
	column col(int idx) const;
	column col(const char *name) const;
	column col(const std::string &name) const;

	/**
	 * Access the value of a column in the current result row.
	 *
	 * Supported types are `int`, `int64_t`, `double`, `const char*`,
	 * `std::string` and `sqxx::blob`.
	 *
	 * When receiving values as `const char*` or `sqxx::blob`, the returned
	 * data will refer to storage internal to sqlite and subsequent calls
	 * to `val()` can invalidate the returned data. For more details see the
	 * [documentation of the corresponding sqlite C API functions](http://www.sqlite.org/c3ref/column_blob.html)
	 *
	 * When receiving a value as `std::string`, the data is copied into the
	 * returned `std::string` object.
	 *
	 * TODO: Add a `copy` flag that can be specified to copy values?
	 *
	 * Wraps [`sqlite3_column_*()`](http://www.sqlite.org/c3ref/column_blob.html)
	 */
	template<typename T>
	if_sqxx_db_type<T, T> val(int idx) const;

	template<typename T>
	if_sqxx_db_type<T, T> val(const char *name) const;

	template<typename T>
	if_sqxx_db_type<T, T> val(const std::string &name) const;

	// Statement execution
	/**
	 * Executes a prepared statement or advances to the next row of results.
	 *
	 * This is a low-level function, usually it is preferable to execute a
	 * statement with `run()` and then iterate over the result with `next_row()`
	 * or use the iterator interface:
	 *
	 *     stmt.prepare("SELECT name from persons where id = 123;");
	 *     stmt.run();
	 *     for (auto&& : stmt) {
	 *        std::cout << stmt.val<const char*>(0);
	 *     }
	 *
	 * Wraps [`sqlite3_step()`](http://www.sqlite.org/c3ref/step.html)
	 */
	void step();

	/** Execute a statement */
	void run();

	/** Alias for `run()` as an analog to `connection::query()` */
	void query();

	/** Advance to next result row */
	void next_row();

	/**
	 * Resets a statement so that it can be executed again.
	 *
	 * Any bound parameters are not automatically unbound by this. Use
	 * `statement::clear_bindings()` for that.
	 *
	 * Wraps ['sqlite3_reset()'](http://www.sqlite.org/c3ref/reset.html).
	 */
	void reset();

	// Result row access

	/** Check if all result rows have been processed */
	bool done() const { return completed; }
	operator bool() const { return !completed; }

	class row_iterator : public std::iterator<std::input_iterator_tag, size_t> {
	private:
		statement *s;
		size_t rowidx;

	public:
		explicit row_iterator(statement *a_s = nullptr);

	private:
		void check_complete();

	public:
		size_t operator*() const { return rowidx; }

		row_iterator& operator++();
		// Not reasonably implementable with correct return type:
		void operator++(int) { ++*this; }

		bool operator==(const row_iterator &other) const { return (s == other.s); }
		bool operator!=(const row_iterator &other) const { return !(*this == other); }
	};

	/**
	 * Iterate over the result of query
	 */
	row_iterator begin() { return row_iterator(this); }
	row_iterator end() { return row_iterator(); }

	/**
	 * Receive statement SQL
	 *
	 * Wraps [`sqlite3_sql()`](http://www.sqlite.org/c3ref/sql.html)
	 **/
	const char* sql();

	/**
	 * Wraps [`sqlite3_stmt_status()`](http://www.sqlite.org/c3ref/stmt_status.html)
	 */
	int status(int op, bool reset=false);

	int status_fullscan_step(bool reset=false);
	int status_sort(bool reset=false);
	int status_autoindex(bool reset=false);

	/**
	 * Determines if the prepared statement writes to the database.
	 *
	 * Wraps [`sqlite3_stmt_readonly()`](http://www.sqlite.org/c3ref/stmt_readonly.html)
	 */
	bool readonly() const;
	/**
	 * Determine if the prepared statement has been reset
	 *
	 * Wraps [`sqlite3_stmt_busy()`](http://www.sqlite.org/c3ref/stmt_busy.html)
	 */
	bool busy() const;

	/** Raw access to the underlying `sqlite3_stmt*` handle */
	sqlite3_stmt* raw() const { return handle; }
};

} // namespace sqxx

#include "statement.impl.hpp"

#endif // SQXX_STATEMENT_HPP_INCLUDED

