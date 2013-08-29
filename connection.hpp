
// (c) 2013 Stephan Hohe

#if !defined(SQXX_CONNECTION_HPP_INCLUDED)
#define SQXX_CONNECTION_HPP_INCLUDED

#include "datatypes.hpp"
#include <memory>
#include <functional>

struct sqlite3;

namespace sqxx {

class statement;
namespace detail {
	// Helpers for user defined callbacks/sql functions
	struct callback_table;
	struct function_data;
}

/** Metadata for a table column */
struct column_metadata {
	const char *datatype;
	const char *collseq;
	bool notnull;
	bool primarykey;
	bool autoinc;
};

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

	/** sqlite3_open_v2() */
	void open(const char *filename, int flags = 0);
	void open(const std::string &filename, int flags = 0) { open(filename.c_str(), flags); }

	/** sqlite3_close_v2() */
	void close() noexcept;
	/** sqlite3_close() */
	void close_sync();

	/** sqlite3_create_collation_v2() */
	typedef std::function<int (int, const char*, int, const char*)> collation_function_t;
	void create_collation(const char *name, const collation_function_t &coll);

	//typedef std::function<int (const std::string &, const std::string &)> collation_str_function_t;
	//void create_collation(const char *name, const collation_str_function_t &coll);

	/** Create a sql prepared statement
	 *
	 * sqlite3_prepare_v2()
	 */
	statement prepare(const char *sql);
	statement prepare(const std::string &sql);

	/** Execute sql
	 *
	 * Returns a `statement` in case you are interested
	 * in the results returned by the query.
	 *
	 * If you are not interested in it, just ignore the return value.
	 */
	statement run(const char *sql);
	statement run(const std::string &sql);

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
	void create_function_p(const char *name, int nargs, detail::function_data *fundata);

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

} // namespace sqxx

#endif // SQXX_CONNECTION_HPP_INCLUDED

