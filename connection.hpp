
// (c) 2013 Stephan Hohe

#if !defined(SQXX_CONNECTION_HPP_INCLUDED)
#define SQXX_CONNECTION_HPP_INCLUDED

#include "datatypes.hpp"
#include "error.hpp"
#include <memory>
#include <functional>

struct sqlite3;

namespace sqxx {

enum open_flags {
	 OPEN_READONLY =         0x00000001,  /* Ok for sqlite3_open_v2() */
	 OPEN_READWRITE =        0x00000002,  /* Ok for sqlite3_open_v2() */
	 OPEN_CREATE =           0x00000004,  /* Ok for sqlite3_open_v2() */
	 //OPEN_DELETEONCLOSE =    0x00000008,  /* VFS only */
	 //OPEN_EXCLUSIVE =        0x00000010,  /* VFS only */
	 //OPEN_AUTOPROXY =        0x00000020,  /* VFS only */
	 OPEN_URI =              0x00000040,  /* Ok for sqlite3_open_v2() */
	 OPEN_MEMORY =           0x00000080,  /* Ok for sqlite3_open_v2() */
	 //OPEN_MAIN_DB =          0x00000100,  /* VFS only */
	 //OPEN_TEMP_DB =          0x00000200,  /* VFS only */
	 //OPEN_TRANSIENT_DB =     0x00000400,  /* VFS only */
	 //OPEN_MAIN_JOURNAL =     0x00000800,  /* VFS only */
	 //OPEN_TEMP_JOURNAL =     0x00001000,  /* VFS only */
	 //OPEN_SUBJOURNAL =       0x00002000,  /* VFS only */
	 //OPEN_MASTER_JOURNAL =   0x00004000,  /* VFS only */
	 OPEN_NOMUTEX =          0x00008000,  /* Ok for sqlite3_open_v2() */
	 OPEN_FULLMUTEX =        0x00010000,  /* Ok for sqlite3_open_v2() */
	 OPEN_SHAREDCACHE =      0x00020000,  /* Ok for sqlite3_open_v2() */
	 OPEN_PRIVATECACHE =     0x00040000,  /* Ok for sqlite3_open_v2() */
	 //OPEN_WAL =              0x00080000,  /* VFS only */
};

class recent_error : public error {
public:
	recent_error(sqlite3 *handle);
};

class statement;

namespace detail {
	// Helpers for user defined callbacks/sql functions
	class connection_callback_table;
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

	friend class detail::connection_callback_table;
	std::unique_ptr<detail::connection_callback_table> callbacks;

	// On-demand initialization of callback table
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

	/**
	 * The database file name.
	 *
	 * Wraps [`sqlite3_db_filename()`](http://www.sqlite.org/c3ref/db_filename.html)
	 */
	const char* filename(const char *db = "main") const;
	const char* filename(const std::string &db) const;

	/**
	 * Database configuration SQLITE_DBCONFIG_LOOKASIDE.
	 *
	 * Wraps [`sqlite3_db_config(s, SQLITE_DBCONFIG_LOOKASIDE, ...)`](http://www.sqlite.org/c3ref/db_config_enable_fkey.html)
	 */
	void config_lookaside(void *buf, int slotsize, int slotcount);

	/**
	 * Database configuration SQLITE_DBCONFIG_ENABLE_FKEY.
	 *
	 * Instead of using a `int*` out-parameter like the C API, this function
	 * returns the new configuration satte as a boolean.
	 *
	 * Wraps [`sqlite3_db_config(s, SQLITE_DBCONFIG_ENABLE_FKEY, ...)`](http://www.sqlite.org/c3ref/db_config_enable_fkey.html)
	 */
	bool config_enable_fkey(int fk);

	/**
	 * Database configuration SQLITE_DBCONFIG_ENABLE_TRIGGER.
	 *
	 * Instead of using a `int*` out-parameter like the C API, this function
	 * returns the new configuration satte as a boolean.
	 *
	 * Wraps [`sqlite3_db_config(s, SQLITE_DBCONFIG_ENABLE_TRIGGER, ...)`](http://www.sqlite.org/c3ref/db_config_enable_fkey.html)
	 */
	bool config_enable_trigger(int trig);

	/**
	 * Determine if the database is read-only.
	 *
	 * Wraps [`sqlite3_db_readonly()`](http://www.sqlite.org/c3ref/db_readonly.html)
	 */
	bool readonly(const char *dbname = "main") const;
	bool readonly(const std::string &dbname) const { return readonly(dbname.c_str()); }

	/**
	 * Determine if the database is in autocommit mode.
	 *
	 * Wraps [`sqlite3_get_autocommit()`](http://www.sqlite.org/c3ref/get_autocommit.html)
	 */
	bool autocommit() const;

	uint64_t last_insert_rowid() const;

	/**
	 * Get the database connection status.
	 *
	 * Wraps [`sqlite3_db_status()`](http://www.sqlite.org/c3ref/db_status.html)
	 */
	counter status(int op, bool reset=false);
	counter status_lookaside_used(bool reset=false);
	counter status_cache_used(bool reset=false);
	counter status_schema_used(bool reset=false);
	counter status_stmt_used(bool reset=false);
	counter status_lookaside_hit(bool reset=false);
	counter status_lookaside_miss_size(bool reset=false);
	counter status_lookaside_miss_full(bool reset=false);
	counter status_cache_hit(bool reset=false);
	counter status_cache_miss(bool reset=false);
	counter status_cache_write(bool reset=false);

	/**
	 * Wraps [`sqlite3_table_column_metadata()`](http://www.sqlite.org/c3ref/table_column_metadata.html)
	 */
	column_metadata metadata(const char *db, const char *table, const char *column) const;

	/** Total number of rows modified by statements.
	 *
	 * Wraps [`sqlite3_total_changes()`](http://www.sqlite.org/c3ref/total_changes.html)
	 */
	int total_changes() const;

	/**
	 * Open a new database connection.
	 *
	 * Wraps [`sqlite3_open_v2()`](http://www.sqlite.org/c3ref/open.html)
	 */
	void open(const char *filename, int flags = 0);
	void open(const std::string &filename, int flags = 0);

	/**
	 * Close the database connection (might delay closure and finish it async).
	 *
	 * Wraps [`sqlite3_close_v2()`](http://www.sqlite.org/c3ref/close.html)
	 */
	void close() noexcept;
	/**
	 * Close the database connection (might fail to close and throw).
	 *
	 * Wraps [`sqlite3_close()`](http://www.sqlite.org/c3ref/close.html)
	 */
	void close_sync();

	/**
	 * Create or redefine a SQL collation function.
	 *
	 * Wraps [`sqlite3_create_collation_v2()`](http://www.sqlite.org/c3ref/create_collation.html)
	 */
	template<typename Callable>
	void create_collation(const char *name, Callable &&fun);

	template<typename Callable>
	void create_collation(const std::string &name, Callable &&fun);

	template<typename Function, Function *Fun>
	void create_collation(const char *name);

	template<typename Function, Function *Fun>
	void create_collation(const std::string &name);


	typedef std::function<int (const std::string &, const std::string &)> collation_function_stdstr_t;
	void create_collation_stdstr(const char *name, const collation_function_stdstr_t &coll);

	template<typename Callable>
	void create_collation_stdstr(const char *name, Callable &&coll);

	void remove_collation(const char *name);
	void remove_collation(const std::string &name);

	/**
	 * Create a sql prepared statement
	 *
	 * Wraps [`sqlite3_prepare_v2()`](http://www.sqlite.org/c3ref/prepare.html)
	 */
	statement prepare(const char *sql);
	statement prepare(const std::string &sql);

	/** Runs a sql query.
	 *
	 * Returns a `statement` in case you are interested
	 * in the results returned by the query. If you are not interested
	 * in it, just ignore the return value.
	 *
	 * This runs a single Sql query, not multiple ones.
	 *
	 * (TODO: Check end throw if multiple statements are passed)
	 */
	statement run(const char *sql);
	statement run(const std::string &sql);

	/** Execute Sql.
	 *
	 * Runs one or multiple Sql statements. Ignores and returned rows.
	 * If you are interested in the results of your queries, use `run()`
	 * instead.
	 *
	 * Wraps [`sqlite3_exec()`](http://www.sqlite.org/c3ref/exec.html)
	 */
	void exec(const char *sql);
	void exec(const std::string &sql);

	/**
	 * Adds a callback to exec that is called with query result rows.
	 * Only provided for compatibility with the C Api, better use `run()`
	 * and its returned `statement` to access selected rows in a comfortable
	 * way.
	 *
	 * Wraps [`sqlite3_exec()`](http://www.sqlite.org/c3ref/exec.html)
	 */
	typedef std::function<bool (int, char**, char**)> exec_handler_t;
	void exec(const char *sql, const exec_handler_t &fun);
	void exec(const std::string &sql, const exec_handler_t &fun);

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

	/**
	 * Interrupt a long-running query
	 *
	 * Wraps [`sqlite3_interrupt()`](http://www.sqlite.org/c3ref/interrupt.html)
	 */
	void interrupt();
	/** 
	 * Set run-time limits
	 *
	 * Wraps [`sqlite3_limit()`](http://www.sqlite.org/c3ref/limit.html)
	 */
	int limit(int id, int newValue);

	/**
	 * Sets a busy timeout.
	 *
	 * Wraps [`sqlite3_busy_timeout()`](http://www.sqlite.org/c3ref/busy_timeout.html)
	 */
	void busy_timeout(int ms);
	/**
	 * Try to free memory that is no longer used by the database connection.
	 *
	 * Wraps [`sqlite3_db_release_memory()`](http://www.sqlite.org/c3ref/db_release_memory.html)
	 */
	void release_memory();

	/**
	 * Counts number of rows modified by last executed statement.
	 *
	 * Wraps [`sqlite3_changes()`](http://www.sqlite.org/c3ref/changes.html)
	 */
	int changes() const;

	/**
	 * Register/clear a commit notification callback.
	 *
	 * Wraps [`sqlite3_commit_hook()`](http://www.sqlite.org/c3ref/commit_hook.html)
	 */
	typedef std::function<int ()> commit_handler_t;
	void set_commit_handler(const commit_handler_t &fun);
	void set_commit_handler();

	/**
	 * Register/clear a rollback notification callback.
	 *
	 * Wraps [`sqlite3_rollback_hook()`](http://www.sqlite.org/c3ref/commit_hook.html)
	 */
	typedef std::function<void ()> rollback_handler_t;
	void set_rollback_handler(const rollback_handler_t &fun);
	void set_rollback_handler();

	/**
	 * Register a data change notification callback.
	 *
	 * Wraps [`sqlite3_update_hook()`](http://www.sqlite.org/c3ref/update_hook.html)
	 */
	typedef std::function<void (int, const char*, const char *, int64_t)> update_handler_t;
	void set_update_handler(const update_handler_t &fun);
	void set_update_handler();

	/**
	 * Register a trace callback function
	 *
	 * Wraps [`sqlite3_trace()`](http://www.sqlite.org/c3ref/profile.html)
	 */
	typedef std::function<void (const char*)> trace_handler_t;
	void set_trace_handler(const trace_handler_t &fun);
	void set_trace_handler();

	/**
	 * Register a profiling callback function
	 *
	 * Wraps [`sqlite3_profile()`](http://www.sqlite.org/c3ref/profile.html)
	 */
	typedef std::function<void (const char*, uint64_t)> profile_handler_t;
	void set_profile_handler(const profile_handler_t &fun);
	void set_profile_handler();

	/**
	 * Register a compile-time authorizer callback function
	 *
	 * Wraps [`sqlite3_set_authorizer()`](http://www.sqlite.org/c3ref/set_authorizer.html)
	 */
	typedef std::function<int (int, const char*, const char*, const char*, const char*)> authorize_handler_t;
	void set_authorize_handler(const authorize_handler_t &fun);
	void set_authorize_handler();

	/**
	 * Registers a callback to handle SQLITE_BUSY errors.
	 *
	 * Wraps [`sqlite3_busy_handler()`](http://www.sqlite.org/c3ref/busy_handler.html)
	 */
	typedef std::function<bool (int)> busy_handler_t;
	void set_busy_handler(const busy_handler_t &busy);
	void set_busy_handler();

	/**
	 * Registers a query progress callback
	 *
	 * Wraps [`sqlite3_progress_handler()`](http://www.sqlite.org/c3ref/progress_handler.html)
	 */
	typedef std::function<bool ()> progress_handler_t;
	void set_progress_handler(int n, const progress_handler_t &fun);
	void set_progress_handler();

	/**
	 * Registers a write-ahead log hook
	 *
	 * Wraps [`sqlite3_wal_handler()`](http://www.sqlite.org/c3ref/wal_hook.html)
	 */
	typedef std::function<void (/*connection&,*/ const char*, int)> wal_handler_t;
	void set_wal_handler(const wal_handler_t &fun);
	void set_wal_handler();

	/**
	 * Configure an auto-checkpoint
	 *
	 * Wraps [`sqlite3_wal_autocheckpoint()`](http://www.sqlite.org/c3ref/wal_autocheckpoint.html)
	 */
	void wal_autocheckpoint(int frames);

	/**
	 * Checkpoint a database
	 *
	 * Wraps [`sqlite3_wal_checkpoint_v2()`](http://www.sqlite.org/c3ref/wal_checkpoint_v2.html)
	 */
	std::pair<int, int> wal_checkpoint(const char *dbname, int emode);

	std::pair<int, int> wal_checkpoint_passive(const char *dbname);
	std::pair<int, int> wal_checkpoint_passive(const std::string &dbname);
	std::pair<int, int> wal_checkpoint_full(const char *dbname);
	std::pair<int, int> wal_checkpoint_full(const std::string &dbname);
	std::pair<int, int> wal_checkpoint_restart(const char *dbname);
	std::pair<int, int> wal_checkpoint_restart(const std::string &dbname);

	/**
	 * Wraps [`sqlite3_collation_needed()`](http://www.sqlite.org/c3ref/collation_needed.html)
	 */
	typedef std::function<void (connection&, const char*)> collation_handler_t;
	void set_collation_handler(const collation_handler_t &fun);
	void set_collation_handler();

	// TODO:
	//typedef std::function<void ()> unlock_handler_t;
	//void set_unlock_handler(const unlock_handler_t &fun);
	//void set_unlock_handler();

public:
	/**
	 * Create or redefine a SQL function.
	 *
	 * For example to create an `increment` SQL function form a `addone`
	 * C++ function:
	 *
	 * ```
	 * int addone(int i) { return i+1; }
	 *
	 * conn.create_function("increment", addone);
	 * ```
	 *
	 * If the callable is a function pointer, it is used directly.
	 *
	 * If it's a callable object (`std::function<>`, lambda function object, ...),
	 * a copy is made and stored for the lifetime of the sqlite function.
	 *
	 * If you need a reference to the callable object instead, wrap
	 * it with std::ref().
	 *
	 * Wraps [`sqlite3_create_function()`](http://www.sqlite.org/c3ref/create_function.html)
	 */
	template<typename Callable>
	void create_function(const char *name, Callable &&fun);

	template<typename Callable>
	void create_function(const std::string &name, Callable &&fun);

	/**
	 * Create or redefine a SQL function.
	 *
	 * This overload takes the function as a template parameter. You need to
	 * specify the function type explicitly. For example, to create an `increment`
	 * SQL function form a `addone`  C++ function:
	 *
	 * ```
	 * int addone(int i) { return i+1; }
	 *
	 * conn.create_function<int (int), addone>("increment");
	 * ```
	 *
	 * This creates slightly more efficient SQL functions than passing a function
	 * pointer as parameter to the other overloades since `addone` is known at
	 * compile time and can be inlined/...
	 *
	 * Wraps [`sqlite3_create_function()`](http://www.sqlite.org/c3ref/create_function.html)
	 */
	template<typename Function, Function *Fun>
	void create_function(const char *name);

	template<typename Function, Function *Fun>
	void create_function(const std::string &name);

	/** TODO:
	 * Define a SQL function with variable number of arguments. The Callable will be
	 * called with a single parameter, a std::vector<sqxx::value>.
	 */
	template<typename Callable>
	void create_function_vararg(const char *name, Callable &&f);

	void remove_function(const char *name, int argc);
	void remove_function(const std::string &name, int argc);

public:
	/**
	 * Register an aggregation function.
	 *
	 * An aggregate function can be used to calculate a combined value
	 * over all the rows in a Sql query result.
	 *
	 * An aggregate calculation consists of some internal state and two
	 * functions, `step_fun` and `final_fun`, that calculate the aggregated value.
	 * The `step_fun` function is called for each value that should be aggregated
	 * and modifies the internal state, `final_fun` is called once in
	 * the end and calculates the final result from the internal state (if
	 * necessary).
	 *
	 * An aggregate definition requires three parts:
	 * - `zero`: A state value that is used as an initial value in aggregate
	 *   calculations. This can be an arbitrary copyable type `State`.
	 * - `step_fun`: A stepping function, that is called for each value that should
	 *   be aggregated. It should have a signature like `void step_fun(State&, Value val)`
	 *   and modify the state passed as first parameter. `Value` should be a type that
	 *   `sqxx::value` can be converted to.
	 *   Multiple `Value` arguments are also possible for multi-parameter aggregates.
	 * - `final_fun`: A finalization function that is called in the end to produce
	 *   the final result. It should have a signature like `Result final_fun(const State&)`
	 *   where `Result` is a type that can be converted to `sqxx::value`.
	 *
	 * For example to add a function that calculates the 
	 * ```
	 * struct geom_state { int64_t sum = 0; int64_t count = 0; };
	 *
	 * auto geom_step = [](geom_state &state, int64_t n) {
	 *    state.count++;
	 *    state.sum += n*n;
	 * };
	 *
	 * auto geom_final = [](const geom_state &state) -> double {
	 * 	return sqrt(state.sum * 1.0 / state.count);
	 * }
	 *
	 * conn.create_aggregate("geom", geom_state(), geom_step, geom_final);
	 * conn.run("SELECT geom(v) FROM values");
	 * ```
	 *
	 * Wraps [`sqlite3_create_function()`](http://www.sqlite.org/c3ref/create_function.html)
	 */
	template<typename State, typename StepCallable, typename FinalCallable>
	void create_aggregate(const char *name, State &&zero, StepCallable &&step_fun,
			FinalCallable &&final_fun);

	template<typename State, typename StepCallable, typename FinalCallable>
	void create_aggregate(const std::string &name, State &&zero, StepCallable &&step_fun,
			FinalCallable &&final_fun);

	/**
	 * Register an aggregation function without a special `final_fun` function.
	 *
	 * The accumulated state is returned directly after the final `step_fun` call.
	 */
	template<typename State, typename StepCallable>
	void create_aggregate(const char *name, State &&zero, StepCallable &&step_fun);

	template<typename State, typename StepCallable>
	void create_aggregate(const std::string &name, State &&zero, StepCallable &&step_fun);

	void remove_aggregate(const char *name, int argc);
	void remove_aggregate(const std::string &name, int argc);

	/** Raw access to the underlying `sqlite3*` handle */
	sqlite3* raw() { return handle; }
};

} // namespace sqxx

#include "connection.impl.hpp"
#include "connection_callbacks.impl.hpp"
#include "connection_create_function.impl.hpp"
#include "connection_create_aggregate.impl.hpp"
#include "connection_create_collation.impl.hpp"

#endif // SQXX_CONNECTION_HPP_INCLUDED

