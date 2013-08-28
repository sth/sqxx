
// (c) 2013 Stephan Hohe

#if !defined(SQXX_SQXX_HPP_INCLUDED)
#define SQXX_SQXX_HPP_INCLUDED

#include "datatypes.hpp"
#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#include <utility>

// structs from <sqlite3.h>
struct sqlite3;
struct sqlite3_stmt;

namespace sqxx {

/** An error thrown if some sqlite API function returns an error */
class error : public std::runtime_error {
public:
	int code;
	error(int code_arg, const char *what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
	error(int code_arg, const std::string &what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
};


class statement;


/**
 * Represents a parameter of a prepared statement
 */
class parameter {
public:
	statement &stmt;
	const int idx;

	parameter(statement &a_stmt, int a_idx);

	/** sqlite3_parameter_name() */
	const char* name() const;

	/** Bind a value to this parameter
	 *
	 * For details see statement::bind()
	 */
	void bind();

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(T value);

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(T value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const T &value, bool copy=true);
};



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


namespace detail {
	struct callback_table;
}

/** Metadata for a table column */
struct column_metadata {
	const char *datatype;
	const char *collseq;
	bool notnull;
	bool primarykey;
	bool autoinc;
};

//class query;

namespace detail {
	struct function_data;
}

/** A database connection */
class connection {
private:
	sqlite3 *handle;

	friend class detail::callback_table;
	std::unique_ptr<detail::callback_table> callbacks;

	void setup_callbacks();

public:
	connection();
	explicit connection(const char *filename, int flags = 0);
	explicit connection(const std::string &filename, int flags = 0);
	~connection() noexcept;

	// Don't copy, move
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;
	connection(connection&&) = default;
	connection& operator=(connection&&) = default;

	/** sqlite3_db_filename() */
	const char* filename(const char *db = "main") const;
	const char* filename(const std::string &db) const { return filename(db.c_str()); }
	/** sqlite3_db_readonly() */
	bool readonly(const char *dbname = "main") const;
	bool readonly(const std::string &dbname) const { return readonly(dbname.c_str()); }
	/** sqlite3_db_status() */
	std::pair<int, int> status(int op, bool reset=false);
	/** sqlite3_table_column_metadata() */
	column_metadata metadata(const char *db, const char *table, const char *column) const;
	/** sqlite3_total_changes() */
	int total_changes() const;

	void open(const char *filename, int flags = 0);
	void open(const std::string &filename, int flags = 0) { open(filename.c_str(), flags); }
	void close() noexcept;
	void close_sync();

	typedef std::function<int (int, const char*, int, const char*)> collation_function_t;
	void create_collation(const char *name, const collation_function_t &coll);

	/** Create a sql prepared statement
	 *
	 * sqlite3_prepare_v2()
	 */
	statement prepare(const char *sql);

	/** Execute sql
	 *
	 * Like sqlite3_exec(), but returns a `statement` in case you are interested
	 * in the results returned by the query.
	 *
	 * If you are not interested in it, just ignore the return value.
	 */
	statement exec(const char *sql);
	statement exec(const std::string &sql);

	/*
	template<typename T>
	std::vector<T> simple_result(int colnum) {
		std::vector<T> v;
		for (auto &r : *this) {
			v.push_back(r.col(colnum).val<T>());
		}
		return v;
	}
	*/

	/** sqlite3_interrupt() */
	void interrupt();
	/** sqlite3_limit() */
	int limit(int id, int newValue);

	/** sqlite3_busy_timeout() */
	void busy_timeout(int ms);
	/** sqlite3_db_release_memory() */
	void release_memory();

	/* sqlite3_commit_hook() */
	typedef std::function<int ()> commit_handler_t;
	void set_commit_handler(const commit_handler_t &fun);
	void set_commit_handler();

	/* sqlite3_rollback_hook() */
	typedef std::function<void ()> rollback_handler_t;
	void set_rollback_handler(const rollback_handler_t &fun);
	void set_rollback_handler();

	/* sqlite3_update_hook() */
	typedef std::function<void (int, const char*, const char *, int64_t)> update_handler_t;
	void set_update_handler(const update_handler_t &fun);
	void set_update_handler();

	/* sqlite3_trace() */
	typedef std::function<void (const char*)> trace_handler_t;
	void set_trace_handler(const trace_handler_t &fun);
	void set_trace_handler();

	/* sqlite3_profile() */
	typedef std::function<void (const char*, uint64_t)> profile_handler_t;
	void set_profile_handler(const profile_handler_t &fun);
	void set_profile_handler();

	/* sqlite3_set_authorizer() */
	typedef std::function<int (int, const char*, const char*, const char*, const char*)> authorize_handler_t;
	void set_authorize_handler(const authorize_handler_t &fun);
	void set_authorize_handler();

	/* sqlite3_busy_handler() */
	typedef std::function<bool (int)> busy_handler_t;
	void set_busy_handler(const busy_handler_t &busy);
	void set_busy_handler();

	/* sqlite3_collation_needed() */
	typedef std::function<void (connection&, const char*)> collation_handler_t;
	void set_collation_handler(const collation_handler_t &fun);
	void set_collation_handler();

private:
	void create_function_p(const char *name, detail::function_data *fundata);

public:
	//template<typename Callable>
	//void create_function(const char *name, Callable &fun);

	template<typename R, typename... Ps>
	void create_function(const char *name, const std::function<R (Ps...)> &fun);

	///** Define a SQL function, automatically deduce the number of arguments */
	//template<typename Callable>
	//void create_function(const char *name, Callable f);
	///** Define a SQL function with the given number of arguments. Use if automatic deduction fails. */
	//template<int NArgs, typename Callable>
	//void create_function_n(const char *name, Callable f);
	/** Define a SQL function with variable number of arguments. The Callable will be
	 * called with a single parameter, a std::vector<sqxx::value>.
	 */
	template<typename Callable>
	void create_function_vararg(const char *name, Callable f);

	/*
	// TODO
	template<typename Aggregator>
	void create_aggregare(const char *name);

	template<typename AggregatorFactory>
	void create_aggregare(const char *name, AggregatorFactory f);
	*/

	/** Raw access to the underlying `sqlite3*` handle */
	sqlite3* raw() { return handle; }
};


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
	parameter param(const std::string &name) { return param(name.c_str()); }

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

	column col(int idx);
	//column<void> col(const char* idx);

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

	int changes() const;

	/** sqlite3_sql() */
	const char* sql();

	/** sqlite3_stmt_status() */
	int status(int op, bool reset=false);

	/** Raw access to the underlying `sqlite3_stmt*` handle */
	sqlite3_stmt* raw() { return handle; }
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



inline void parameter::bind() { stmt.bind(idx); }

template<typename T>
if_selected_type<T, void, int, int64_t, double>
parameter::bind(T value) { stmt.bind<T>(idx, value); }

template<typename T>
if_selected_type<T, void, const char*>
parameter::bind(T value, bool copy) { stmt.bind<T>(idx, value, copy); }

template<typename T>
if_selected_type<T, void, std::string, blob>
parameter::bind(const T &value, bool copy) { stmt.bind<T>(idx, value, copy); }

template<typename T>
if_sqxx_db_type<T, T> column::val() const {
	return stmt.val<T>(idx);
}

/** sqlite3_status() */
std::pair<int, int> status(int op, bool reset=false);


// Handle exceptions thrown from C++ callbacks
typedef std::function<void (const char*, std::exception_ptr)> callback_exception_handler_t;
void set_callback_exception_handler(const callback_exception_handler_t &handler);

// Write exeptions to std::cerr
void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept;

} // namespace sqxx

#endif // SQXX_SQXX_HPP_INCLUDED

