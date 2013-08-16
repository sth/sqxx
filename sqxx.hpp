
// (c) 2013 Stephan Hohe

#if !defined(SQXX_INCLUDED)
#define SQXX_INCLUDED

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>

// structs from <sqlite3.h>
struct sqlite3;
struct sqlite3_stmt;

namespace sqxx {

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

/*
template<typename T>
typename std::enable_if<std::is_pod<T>::value, std::vector<T>>::type to_vector(const blob &b) {
	const T *begin = reinterpret_cast<const T*>(b.first);
	const T *end = begin + (b.second / sizeof(T));
	return std::vector<T>(begin, end);
}
*/

class statement;

class parameter {
public:
	statement &stmt;
	int idx;

	parameter(statement &a_stmt, int a_idx);

	const char* name() const;

	void bind(int value);
	void bind(int64_t value);
	void bind(double value);
	void bind(const char *value, bool copy = true);
	void bind(const std::string &value, bool copy = true) { bind(value.c_str(), copy); }
	void bind(const blob &value, bool copy = true);
	template<typename T>
	void bind(const std::vector<T> &value, bool copy = true) { bind(vector_blob(value), copy); }
	void bind_zeroblob(int len);

	parameter& operator=(int value) { bind(value); return *this; }
	parameter& operator=(int64_t value) { bind(value); return *this; }
	parameter& operator=(double value) { bind(value); return *this; }
	parameter& operator=(const char *value) { bind(value, true); return *this; }
	parameter& operator=(const std::string &value) { bind(value, true); return *this; }
	parameter& operator=(const blob &value) { bind(value, true); return *this; }
	template<typename T>
	parameter& operator=(const std::vector<T> &value) { bind(vector_blob(value), true); return *this; }
};


class column {
public:
	statement &stmt;
	const int idx;

	column(statement &a_stmt, int a_idx);

	const char* name() const;
	const char* database_name() const;
	const char* table_name() const;
	const char* origin_name() const;

	int type() const;
	const char* decl_type() const;

	template<typename T>
	T val() const;

	template<typename T>
	std::vector<T> vec() const {
		return blob_vector<T>(val<blob>());
	}

	operator int() const;
	operator int64_t() const;
	operator double() const;
	operator const char*() const;
	//operator std::string() const;
	operator blob() const;
	template<typename T>
	operator std::vector<T>() const;
};

template<> int column::val<int>() const;
template<> int64_t column::val<int64_t>() const;
template<> double column::val<double>() const;
template<> const char* column::val<const char*>() const;
template<> inline std::string column::val<std::string>() const { return val<const char*>(); }
template<> blob column::val<blob>() const;
/* No partial specialization of function templates
template<typename T>
std::vector<T> column::val<std::vector<T>>() const {
	return blob_vector<T>(val<blob>());
}
*/

inline column::operator int() const { return val<int>(); }
inline column::operator int64_t() const { return val<int64_t>(); }
inline column::operator double() const { return val<double>(); }
inline column::operator const char*() const { return val<const char*>(); }
//inline column::operator std::string() const { return val<std::string>(); }
inline column::operator blob() const { return val<blob>(); }
template<typename T>
column::operator std::vector<T>() const { return vec<std::vector<T>>(); }


namespace detail {
	struct callback_table;
}

struct column_metadata {
	const char *datatype;
	const char *collseq;
	bool notnull;
	bool primarykey;
	bool autoinc;
};

//class query;

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

	typedef std::function<int (size_t, const char*, size_t, const char*)> collation_function_t;
	void create_collation(const char *name, const collation_function_t &coll);

	/** Create a sql statement
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
	template<typename Aggregator>
	void create_aggregare(const char *name);

	template<typename AggregatorFactory>
	void create_aggregare(const char *name, AggregatorFactory f);
	*/

	sqlite3* raw() { return handle; }
};


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

	const char* sql();
	/** sqlite3_stmt_status() */
	int status(int op, bool reset=false);

	// Don't copy, move
	statement(const statement &) = delete;
	statement& operator=(const statement&) = delete;
	statement(statement &&) = default;
	statement& operator=(statement&&) = default;

	int param_index(const char *name) const;
	int param_index(const std::string &name) const { return param_index(name.c_str()); }
	int param_count() const;

	parameter param(int idx);
	parameter param(const char *name);
	parameter param(const std::string &name) { return param(name.c_str()); }

	int col_count() const;
	column col(int idx);

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


	sqlite3_stmt* raw() { return handle; }
};


// Handle exceptions thrown from C++ callbacks
typedef std::function<void (const char*, std::exception_ptr)> callback_exception_handler_t;
void set_callback_exception_handler(const callback_exception_handler_t &handler);

// Write exeptions to std::cerr
void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept;

} // namespace sqxx

#endif // SQXX_INCLUDED

