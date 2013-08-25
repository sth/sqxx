
// (c) 2013 Stephan Hohe

#if !defined(SQXX_INCLUDED)
#define SQXX_INCLUDED

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#include <utility>

// structs from <sqlite3.h>
struct sqlite3;
struct sqlite3_stmt;

namespace sqxx {

/** A error thrown if some sqlite API function returns an error */
class error : public std::runtime_error {
public:
	int code;
	error(int code_arg, const char *what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
	error(int code_arg, const std::string &what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
};



/** sqlite3_status() */
std::pair<int, int> status(int op, bool reset=false);


// TODO: blob_iterator?

typedef std::pair<const void*, int> blob;

inline blob make_blob(const void *data, int len) {
	return std::make_pair(data, len);
}

// TODO: Decide if this is really a good idea
/*
template<typename T>
inline blob make_blob(const std::vector<T> &data) {
	return vector_blob(data);
}

template<typename T>
typename std::enable_if<
	std::is_pod<T>::value,
	std::vector<T>
>::type blob_vector(const blob &b) {
	const T *begin = reinterpret_cast<const T*>(b.first);
	const T *end = begin + (b.second / sizeof(T));
	return std::vector<T>(begin, end);
}

template<typename T>
typename std::enable_if<
	std::is_pod<T>::value,
	blob
>::type vector_blob(const std::vector<T> &v) {
	return std::make_pair(v.data(), v.size() * sizeof(T));
}
*/

/*
template<typename T>
typename std::enable_if<std::is_pod<T>::value, std::vector<T>>::type to_vector(const blob &b) {
	const T *begin = reinterpret_cast<const T*>(b.first);
	const T *end = begin + (b.second / sizeof(T));
	return std::vector<T>(begin, end);
}
*/

namespace detail {

// Check if type T is any of the other ones listed in Ts...
template<typename T, typename... Ts>
struct is_selected {
	static const bool value = false;
};

template<typename T, typename T2, typename... Ts>
struct is_selected<T, T2, Ts...> {
	static const bool value = std::is_same<T, T2>::value || is_selected<T, Ts...>::value;
};

}

/** `std::enable_if<>` for all types in the `Ts...` list */
template<typename T, typename R, typename... Ts>
using if_selected_type = typename std::enable_if<detail::is_selected<T, Ts...>::value, R>::type;

/** `std::enable_if<>` for the types supported by sqxx's db interface
 * (`int`, `int64_t`, `double`, `const char*`, `std::string`, `blob`)
 */
template<typename T, typename R>
using if_sqxx_db_type = if_selected_type<T, R,
		int, int64_t, double, const char*, std::string, blob>;


class statement;

/**
 * Represents a parameter of a prepared statement
 *
 * Supports binding of values to that parameter.
 */
class parameter {
public:
	statement &stmt;
	const int idx;

	parameter(statement &a_stmt, int a_idx);

	const char* name() const;

	/* Binds a NULL
	 *
	 * sqlite3_bind_null()
	 */
	void bind();

	/** Templated versions of bind()
	 *
	 * Allows to select a certain 
	 */

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

/** Set the parameter to a int value.
 *
 * sqlite3_bind_int()
 */
template<>
void parameter::bind<int>(int value);

/** Set the parameter to a int64_t value
 *
 * sqlite3_bind_int64()
 */
template<>
void parameter::bind<int64_t>(int64_t value);

/** Set the parameter to a double value
 *
 * sqlite3_bind_double()
 */
template<>
void parameter::bind<double>(double value);

/** Set the parameter to a const char* value.
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
void parameter::bind<const char*>(const char* value, bool copy);

template<>
inline void parameter::bind<std::string>(const std::string &value, bool copy) {
	bind<const char*>(value.c_str(), copy);
}

/** Set the parameter to a blob.
 *
 * If the data pointer of the blob is a null pointer,
 * it creates a blob consisting of zeroes.
 *
 * sqlite3_bind_blob()
 * sqlite3_bind_zeroblob()
 */
template<>
void parameter::bind<blob>(const blob &value, bool copy);

/** A column of a sql query result */

class column_base {
public:
	statement &stmt;
	const int idx;

	column_base(statement &a_stmt, int a_idx);

	const char* name() const;
	const char* database_name() const;
	const char* table_name() const;
	const char* origin_name() const;

	int type() const;
	const char* decl_type() const;

	template<typename T>
	if_sqxx_db_type<T, T> val() const;

	/*
	template<typename T>
	std::vector<T> vec() const {
		return blob_vector<T>(val<blob>());
	}
	*/
};

/** sqlite3_column_int() */
template<> int column_base::val<int>() const;

/** sqlite3_column_int64() */
template<> int64_t column_base::val<int64_t>() const;

/** sqlite3_column_double() */
template<> double column_base::val<double>() const;

/** sqlite3_column_text() */
template<> const char* column_base::val<const char*>() const;
template<> inline std::string column_base::val<std::string>() const {
	return val<const char*>();
}

/** sqlite3_column_blob() */
template<> blob column_base::val<blob>() const;


template<typename T>
class column : public column_base {
public:
	//using column_base::column;
	template <typename... Args>
	column(Args&&... args) : column_base(std::forward<Args>(args)...) {
	}

	using column_base::val;

	T val() const { return val<T>(); }
	operator T() const { return val(); }
};

template<>
class column<void> : public column_base {
public:
	//using column_base::column;
	template <typename... Args>
	column(Args&&... args) : column_base(std::forward<Args>(args)...) {
	}
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

	/** Define a SQL function, automatically deduce the number of arguments */
	template<typename Callable>
	void create_function(const char *name, Callable f);
	/** Define a SQL function with the given number of arguments. Use if automatic deduction fails. */
	template<int NArgs, typename Callable>
	void create_function_n(const char *name, Callable f);
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

	/** Bind parameter values, see parameter::bind() for details.
	 *
	 * The first parameter of each of these functions is an index or a name
	 * of the parameter, the second parameter the value that should be bound.
	 * Parameter indexes start at zero.
	 *
	 *     bind(0, 123);
	 *     bind("name", 345):
	 *
	 * For string and blob parameters a optional third parameter is
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
	void bind(int idx) { param(idx).bind(); }
	void bind(const char *name) { param(name).bind(); }
	void bind(const std::string &name) { param(name).bind(); }

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(int idx, T value) { param(idx).bind<T>(value); }

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const char *name, T value) { param(name).bind<T>(value); }
	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(const std::string &name, T value) { param(name).bind<T>(value); }

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(int idx, T value, bool copy=true) { param(idx).bind<T>(value, copy); }

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const char *name, T value, bool copy=true) { param(name).bind<T>(value, copy); }
	template<typename T>
	if_selected_type<T, void, const char*>
	bind(const std::string &name, T value, bool copy=true) { param(name).bind<T>(value, copy); }

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(int idx, const T &value, bool copy=true) { param(idx).bind<T>(value, copy); }

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const char *name, const T &value, bool copy=true) { param(name).bind<T>(value, copy); }
	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const std::string &name, const T &value, bool copy=true) { param(name).bind<T>(value, copy); }


	// Result columns

	/** sqlite3_column_count() */
	int col_count() const;

	column<void> col(int idx);
	//column<void> col(const char* idx);

	template<typename T>
	if_sqxx_db_type<T, column<T>> col(int idx) {
		return column<T>(*this, idx);
	}

	template<typename T>
	if_sqxx_db_type<T, T> val(int idx) {
		return column<T>(*this, idx).val();
	}


	// Statement execution
	/** sqlite3_step() */
	void step();

	/** Execute a statement */
	void run();
	/** Advance to next result row */
	void next_row();

	//column col(const char *name);

	//int changes();

	void reset();
	void clear_bindings();

	// Result row access

	// TODO: The following functions all do the same thing, remove some
	bool complete() const;
	bool eof() const { return completed; }
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


// Handle exceptions thrown from C++ callbacks
typedef std::function<void (const char*, std::exception_ptr)> callback_exception_handler_t;
void set_callback_exception_handler(const callback_exception_handler_t &handler);

// Write exeptions to std::cerr
void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept;

} // namespace sqxx

#endif // SQXX_INCLUDED

