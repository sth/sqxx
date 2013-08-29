
// (c) 2013 Stephan Hohe

#if !defined(SQXX_STATEMENT_HPP_INCLUDED)
#define SQXX_STATEMENT_HPP_INCLUDED

#include "connection.hpp"

// struct from <sqlite3.h>
struct sqlite3_stmt;

namespace sqxx {

class parameter;
class column;

/**
 * A sql statement
 *
 * Includes:
 *
 * - Binding of parameters for prepared statements
 * - Accessing of result rows after executing the statement
 */
class statement {
private:
	sqlite3_stmt *handle;

public:
	connection &conn;

protected:
	bool completed;

public:
	statement(connection &c, sqlite3_stmt *a_handle);
	~statement();

	// Don't copy, move
	statement(const statement &) = delete;
	statement& operator=(const statement&) = delete;
	statement(statement &&) = default;
	statement& operator=(statement&&) = default;

	// Parameters of prepared statements

	/** sqlite3_bind_parameter_count() */
	int param_count() const;

	/** sqlite3_bind_parameter_index() */
	int param_index(const char *name) const;
	int param_index(const std::string &name) const { return param_index(name.c_str()); }

	/** Get a parameter object, by index or name */
	parameter param(int idx);
	parameter param(const char *name);
	parameter param(const std::string &name);

	/** Bind parameter values in prepared statements
	 *
	 * For each supported type an appropriate specialization is provided,
	 * calling the underlying C API function.
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
	 *     bind("name", 345):
	 *
	 * For string and blob parameters an optional third parameter is
	 * available, that specifies if sqlite should make an internal copy of
	 * the passed value. If the parameter is false and no copy is created, the
	 * caller has to make sure that the passed value stays alive and isn't
	 * destroyed until the query is completed. Default is to create copies
	 * of the passed parameters.
	 *
	 *    bind(0, std::string("temporary"), true);
	 *    bind(1, "persistent", false);
	 *
	 * If the passed value isn't exactly one supported by sqlite3, overload
	 * resolution can be ambigious, which leads to compiler errors. In this
	 * case manually specify the desired parameter type:
	 *
	 *     bind<int64_t>(1, 123U);
	 *
	 */

	/** Set a parameter to a SQL NULL value */
	void bind(int idx);
	void bind(const char *name) { bind(param_index(name)); }
	void bind(const std::string &name) { bind(name.c_str()); }

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(int idx, T value);

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const char *name, T value) { bind<T>(param_index(name), value); }
	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const std::string &name, T value) { bind<T>(name.c_str(), value); }

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(int idx, T value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const char *name, T value, bool copy=true) { bind<T>(param_index(name), value, copy); }
	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const std::string &name, T value, bool copy=true) { bind<T>(name.c_str(), value, copy); }

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(int idx, const T &value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const char *name, const T &value, bool copy=true) { bind<T>(param_index(name), value, copy); }
	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const std::string &name, const T &value, bool copy=true) { bind<T>(name.c_str(), value, copy); }

	/** sqlite3_clear_bindings() */
	void clear_bindings();


	// Result columns

	/** sqlite3_column_count() */
	int col_count() const;

	/** Get a column object */
	column col(int idx);

	/** Access the value of a column in the current result row. */
	template<typename T>
	if_sqxx_db_type<T, T> val(int idx) const;


	// Statement execution
	/** sqlite3_step() */
	void step();

	/** Execute a statement */
	void run();
	/** Advance to next result row */
	void next_row();

	/** sqlite3_reset() */
	void reset();

	// Result row access

	/** Check if all result rows have been processed */
	bool done() const { return completed; }
	operator bool() const { return !completed; }

	class row_iterator : public std::iterator<std::input_iterator_tag, statement> {
	private:
		statement *s;

	public:
		explicit row_iterator(statement *a_s = nullptr);

	private:
		void check_complete();

	public:
		statement& operator*() const { return *s; }

		row_iterator& operator++();
		// Not reasonably implementable with correct return:
		void operator++(int) { ++*this; }

		bool operator==(const row_iterator &other) const { return (s == other.s); }
		bool operator!=(const row_iterator &other) const { return !(*this == other); }
	};

	row_iterator begin() { return row_iterator(this); }
	row_iterator end() { return row_iterator(); }

	/** sqlite3_changes() */
	int changes() const;

	/** sqlite3_sql() */
	const char* sql();

	/** sqlite3_stmt_status() */
	int status(int op, bool reset=false);

	/** Raw access to the underlying `sqlite3_stmt*` handle */
	sqlite3_stmt* raw() const { return handle; }
};


/** Set a parameter to an int value
 *
 * sqlite3_bind_int()
 */
template<>
void statement::bind<int>(int idx, int value);

/** Set a parameter to an int64_t value
 *
 * sqlite3_bind_int64()
 */
template<>
void statement::bind<int64_t>(int idx, int64_t value);

/** Set a parameter to a double value
 *
 * sqlite3_bind_double()
 */
template<>
void statement::bind<double>(int idx, double value);

/** Set a parameter to a const char* value.
 *
 * With copy=false, tells sqlite to not make an internal copy of the value.
 * This means that the value passed in has to stay available until
 * the query is completed.
 *
 * If `value` is a null pointer, this binds a NULL.
 *
 * sqlite3_bind_text()
 */
template<>
void statement::bind<const char*>(int idx, const char* value, bool copy);

template<>
inline void statement::bind<std::string>(int idx, const std::string &value, bool copy) {
	bind<const char*>(idx, value.c_str(), copy);
}

/** Set a parameter to a blob.
 *
 * If the data pointer of the blob is a null pointer,
 * it creates a blob consisting of zeroes.
 *
 * sqlite3_bind_blob()
 * sqlite3_bind_zeroblob()
 */
template<>
void statement::bind<blob>(int idx, const blob &value, bool copy);


/** sqlite3_column_int() */
template<>
int statement::val<int>(int idx) const;

/** sqlite3_column_int64() */
template<>
int64_t statement::val<int64_t>(int idx) const;

/** sqlite3_column_double() */
template<>
double statement::val<double>(int idx) const;

/** sqlite3_column_text() */
template<>
const char* statement::val<const char*>(int idx) const;

template<>
inline std::string statement::val<std::string>(int idx) const {
	return val<const char*>(idx);
}

/** sqlite3_column_blob() */
template<>
blob statement::val<blob>(int idx) const;

} // namespace sqxx

#endif // SQXX_STATEMENT_HPP_INCLUDED

