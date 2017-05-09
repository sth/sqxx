
// (c) 2013 Stephan Hohe

#include "connection.hpp"
#include "sqxx.hpp"
#include "error.hpp"
#include <sqlite3.h>
#include <cstring>

namespace sqxx {

recent_error::recent_error(sqlite3 *handle) : error(sqlite3_errcode(handle), sqlite3_errmsg(handle)) {
}

struct collation_data_t {
	connection &conn;
	connection::collation_handler_t fn;
	collation_data_t(connection &conn_arg, const connection::collation_handler_t &fn_arg)
		: conn(conn_arg), fn(fn_arg) {
	}
};

namespace detail {

class connection_callback_table {
public:
	std::unique_ptr<connection::commit_handler_t> commit_handler;
	std::unique_ptr<connection::rollback_handler_t> rollback_handler;
	std::unique_ptr<connection::update_handler_t> update_handler;
	std::unique_ptr<connection::trace_handler_t> trace_handler;
	std::unique_ptr<connection::profile_handler_t> profile_handler;
	std::unique_ptr<connection::authorize_handler_t> authorize_handler;
	std::unique_ptr<connection::busy_handler_t> busy_handler;
	std::unique_ptr<connection::progress_handler_t> progress_handler;
	std::unique_ptr<connection::wal_handler_t> wal_handler;
	std::unique_ptr<collation_data_t> collation_data;
};

void create_function_register(sqlite3 *handle, const char *name, int argc,
		void *data, sqxx_function_call_type *fun, sqxx_appdata_destroy_type *destroy) {
	int rv = sqlite3_create_function_v2(handle, name, argc, SQLITE_UTF8, data,
			fun, nullptr, nullptr, destroy);
	if (rv != SQLITE_OK) {
		// Registered destructor is called automatically, no need to delete |adat| ourselves
		throw static_error(rv);
	}
}

void create_aggregate_register(sqlite3 *handle, const char *name, int nargs, void *adat,
		sqxx_aggregate_step_type *stepfun, sqxx_aggregate_final_type *finalfun,
		sqxx_appdata_destroy_type *destroy) {
	int rv = sqlite3_create_function_v2(handle, name, nargs, SQLITE_UTF8, adat,
			nullptr, stepfun, finalfun, destroy);
	if (rv != SQLITE_OK) {
		// Registered destructor is called automatically, no need to delete |adat| ourselves
		throw static_error(rv);
	}
}

void create_collation_register(sqlite3 *handle, const char *name, void *data,
		sqxx_collation_compare_type *comparefun, sqxx_appdata_destroy_type *destroy) {
	int rv = sqlite3_create_collation_v2(handle, name, SQLITE_UTF8, data, comparefun, destroy);
	if (rv != SQLITE_OK) {
		if (destroy)
			destroy(data);
		throw static_error(rv);
	}
}

} // namespace detail


// ---------------------------------------------------------------------------
// connection

connection::connection() : handle(nullptr) {
}

connection::connection(const char *filename, int flags) : handle(nullptr) {
	open(filename, flags);
}

connection:: connection(const std::string &filename, int flags) : handle(nullptr) {
	open(filename, flags);
}

connection::~connection() noexcept {
	if (handle) {
		close();
	}
}

const char* connection::filename(const char *db) const {
	return sqlite3_db_filename(handle, db);
}

const char* connection::filename(const std::string &db) const {
	return filename(db.c_str());
}

void connection::config_lookaside(void *buf, int slotsize, int slotcount) {
	int rv = sqlite3_db_config(handle, SQLITE_DBCONFIG_LOOKASIDE, buf, slotsize, slotcount);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

bool connection::config_enable_fkey(int fk) {
	int state;
	int rv = sqlite3_db_config(handle, SQLITE_DBCONFIG_ENABLE_FKEY, fk, &state);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return state;
}

bool connection::config_enable_trigger(int trig) {
	int state;
	int rv = sqlite3_db_config(handle, SQLITE_DBCONFIG_ENABLE_TRIGGER, trig, &state);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return state;
}

bool connection::readonly(const char *dbname) const {
	int rw = sqlite3_db_readonly(handle, dbname);
	if (rw == -1)
		throw error(SQLITE_ERROR, "no such database");
	return bool(rw);
}

bool connection::autocommit() const {
	return sqlite3_get_autocommit(handle);
}

uint64_t connection::last_insert_rowid() const {
	return sqlite3_last_insert_rowid(handle);
}

counter connection::status(int op, bool reset) {
	int rv;
	counter result;
	rv = sqlite3_db_status(handle, op, &result.current, &result.highwater, static_cast<int>(reset));
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return result;
}

counter connection::status_lookaside_used(bool reset) {
	return status(SQLITE_DBSTATUS_LOOKASIDE_USED, reset);
}

counter connection::status_cache_used(bool reset) {
	return status(SQLITE_DBSTATUS_CACHE_USED, reset);
}

counter connection::status_schema_used(bool reset) {
	return status(SQLITE_DBSTATUS_SCHEMA_USED, reset);
}

counter connection::status_stmt_used(bool reset) {
	return status(SQLITE_DBSTATUS_STMT_USED, reset);
}

counter connection::status_lookaside_hit(bool reset) {
	return status(SQLITE_DBSTATUS_LOOKASIDE_HIT, reset);
}

counter connection::status_lookaside_miss_size(bool reset) {
	return status(SQLITE_DBSTATUS_LOOKASIDE_MISS_SIZE, reset);
}

counter connection::status_lookaside_miss_full(bool reset) {
	return status(SQLITE_DBSTATUS_LOOKASIDE_MISS_FULL, reset);
}

counter connection::status_cache_hit(bool reset) {
	return status(SQLITE_DBSTATUS_CACHE_HIT, reset);
}

counter connection::status_cache_miss(bool reset) {
	return status(SQLITE_DBSTATUS_CACHE_MISS, reset);
}

counter connection::status_cache_write(bool reset) {
	return status(SQLITE_DBSTATUS_CACHE_WRITE, reset);
}

column_metadata connection::metadata(const char *db, const char *table, const char *column) const {
	int rv;
	int notnull, primarykey, autoinc;
	column_metadata md;
	rv = sqlite3_table_column_metadata(handle, db, table, column, &md.datatype, &md.collseq, &notnull, &primarykey, &autoinc);
	if (rv != SQLITE_OK)
		throw recent_error(handle);
	md.notnull = notnull;
	md.primarykey = primarykey;
	md.autoinc = autoinc;
	return md;
}

int connection::total_changes() const {
	return sqlite3_total_changes(handle);
}

void connection::open(const char *filename, int flags) {
	int rv;
	if (!flags)
		flags = OPEN_READWRITE;
	rv = sqlite3_open_v2(filename, &handle, flags, nullptr);
	if (rv != SQLITE_OK) {
		if (handle)
			close();
		throw static_error(rv);
	}
}

void connection::open(const std::string &filename, int flags) {
	open(filename.c_str(), flags);
}

void connection::close_sync() {
	int rv;
	rv = sqlite3_close(handle);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	handle = nullptr;
}

void connection::close() noexcept {
#if (SQLITE_VERSION_NUMBER >= 3007014)
	sqlite3_close_v2(handle);
#else
	try {
		close_sync();
	}
	catch (const error &e) {
		std::cerr << "error on sqlite3_close(): " << e.what() << std::endl;
		// Nothing we can do
	}
#endif
	handle = nullptr;
}

/*
void connection::create_str_collation(const char *name, const collation_str_function_t &coll) {
	// TODO: in the lambda, coll is a copy of the function or only a copy of the reference?
	create_collation(name, collation_function_t([coll](int llen, const char *lstr, int rlen, const char *rstr) -> int {
		return coll(std::string(lstr, llen), std::string(rstr, rlen));
	}));
}
*/


struct exec_context {
	const connection::exec_handler_t &fun;
	std::exception_ptr ex;
	exec_context(const connection::exec_handler_t &fun_arg) : fun(fun_arg), ex() {
	}
};

int sqxx_call_exec_handler(void *data, int colcount, char **values, char **columns) {
	exec_context *ctx = reinterpret_cast<exec_context*>(data);
	try {
		return ctx->fun(colcount, values, columns);
	}
	catch (...) {
		ctx->ex = std::current_exception();
		return 1;
	}
}

void connection::exec(const char *sql, const exec_handler_t &fun) {
	char *errmsg = nullptr;
	int rv;
	if (fun) {
		exec_context ctx(fun);
		rv = sqlite3_exec(handle, sql, sqxx_call_exec_handler, &ctx, &errmsg);
		if (ctx.ex) {
			std::rethrow_exception(ctx.ex);
		}
	}
	else {
		rv = sqlite3_exec(handle, sql, nullptr, nullptr, &errmsg);
	}

	if (rv != SQLITE_OK) {
		//TODO: Use errmsg
		sqlite3_free(errmsg);
		throw static_error(rv);
	}
}

statement connection::run(const char *sql) {
	statement st = prepare(sql);
	st.run();
	return st;
}

statement connection::run(const std::string &sql) {
	return run(sql.c_str());
}

void connection::remove_collation(const char *name) {
	int rv = sqlite3_create_collation_v2(handle, name, SQLITE_UTF8,
			nullptr, nullptr, nullptr);
	if (rv != SQLITE_OK) {
		throw static_error(rv);
	}
}

void connection::remove_collation(const std::string &name) {
	remove_collation(name.c_str());
}

statement connection::prepare(const char *sql) {
	int rv;
	sqlite3_stmt *stmt = nullptr;

	rv = sqlite3_prepare_v2(handle, sql, std::strlen(sql)+1, &stmt, nullptr);
	if (rv != SQLITE_OK) {
		throw static_error(rv);
	}

	return statement(*this, stmt);
}

statement connection::prepare(const std::string &sql) {
	return prepare(sql.c_str());
}

void connection::interrupt() {
	sqlite3_interrupt(handle);
}

int connection::limit(int id, int newValue) {
	return sqlite3_limit(handle, id, newValue);
}


void connection::release_memory() {
	sqlite3_db_release_memory(handle);
}

void connection::setup_callbacks() {
	if (!callbacks)
		callbacks.reset(new detail::connection_callback_table);
}

int connection::changes() const {
	return sqlite3_changes(handle);
}

extern "C"
int sqxx_call_commit_handler(void *data) {
	connection::commit_handler_t *fn = reinterpret_cast<connection::commit_handler_t*>(data);
	try {
		return (*fn)();
	}
	catch (...) {
		handle_callback_exception("commit handler");
		return 1;
	}
}

void connection::set_commit_handler(const commit_handler_t &fun) {
	if (fun) {
		std::unique_ptr<commit_handler_t> cb(new commit_handler_t(fun));
		sqlite3_commit_hook(handle, sqxx_call_commit_handler, cb.get());
		setup_callbacks();
		callbacks->commit_handler = std::move(cb);
	}
	else {
		set_commit_handler();
	}
}

void connection::set_commit_handler() {
	sqlite3_commit_hook(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->commit_handler.reset();
}

extern "C"
void sqxx_call_rollback_handler(void *data) {
	connection::rollback_handler_t *fn = reinterpret_cast<connection::rollback_handler_t*>(data);
	try {
		(*fn)();
	}
	catch (...) {
		handle_callback_exception("rollback handler");
	}
}

void connection::set_rollback_handler(const rollback_handler_t &fun) {
	if (fun) {
		std::unique_ptr<rollback_handler_t> cb(new rollback_handler_t(fun));
		sqlite3_rollback_hook(handle, sqxx_call_rollback_handler, cb.get());
		setup_callbacks();
		callbacks->rollback_handler = std::move(cb);
	}
	else {
		set_rollback_handler();
	}
}

void connection::set_rollback_handler() {
	sqlite3_rollback_hook(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->rollback_handler.reset();
}


extern "C"
void sqxx_call_update_handler(void *data, int op, char const *database_name, const char *table_name, sqlite3_int64 rowid) {
	connection::update_handler_t *fn = reinterpret_cast<connection::update_handler_t*>(data);
	try {
		(*fn)(op, database_name, table_name, rowid);
	}
	catch (...) {
		handle_callback_exception("update handler");
	}
}

void connection::set_update_handler(const update_handler_t &fun) {
	if (fun) {
		std::unique_ptr<update_handler_t> cb(new update_handler_t(fun));
		sqlite3_update_hook(handle, sqxx_call_update_handler, cb.get());
		setup_callbacks();
		callbacks->update_handler = std::move(cb);
	}
	else {
		set_update_handler();
	}
}

void connection::set_update_handler() {
	sqlite3_update_hook(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->update_handler.reset();
}

extern "C"
void sqxx_call_trace_handler(void *data, const char* sql) {
	connection::trace_handler_t *fn = reinterpret_cast<connection::trace_handler_t*>(data);
	try {
		(*fn)(sql);
	}
	catch (...) {
		handle_callback_exception("trace handler");
	}
}

void connection::set_trace_handler(const trace_handler_t &fun) {
	if (fun) {
		std::unique_ptr<trace_handler_t> cb(new trace_handler_t(fun));
		sqlite3_trace(handle, sqxx_call_trace_handler, cb.get());
		setup_callbacks();
		callbacks->trace_handler = std::move(cb);
	}
	else {
		set_trace_handler();
	}
}

void connection::set_trace_handler() {
	sqlite3_trace(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->trace_handler.reset();
}

extern "C"
void sqxx_call_profile_handler(void *data, const char* sql, sqlite_uint64 nsec) {
	connection::profile_handler_t *fn = reinterpret_cast<connection::profile_handler_t*>(data);
	try {
		(*fn)(sql, static_cast<uint64_t>(nsec));
	}
	catch (...) {
		handle_callback_exception("profile handler");
	}
}

void connection::set_profile_handler(const profile_handler_t &fun) {
	if (fun) {
		std::unique_ptr<profile_handler_t> cb(new profile_handler_t(fun));
		sqlite3_profile(handle, sqxx_call_profile_handler, cb.get());
		setup_callbacks();
		callbacks->profile_handler = std::move(cb);
	}
	else {
		set_profile_handler();
	}
}

void connection::set_profile_handler() {
	sqlite3_profile(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->profile_handler.reset();
}

extern "C"
int sqxx_call_authorize_handler(void *data, int action, const char* d1, const char *d2, const char *d3, const char *d4) {
	connection::authorize_handler_t *fn = reinterpret_cast<connection::authorize_handler_t*>(data);
	try {
		return (*fn)(action, d1, d2, d3, d4);
	}
	catch (...) {
		handle_callback_exception("authorize handler");
		return SQLITE_IGNORE;
	}
}

void connection::set_authorize_handler(const authorize_handler_t &fun) {
	if (fun) {
		std::unique_ptr<authorize_handler_t> cb(new authorize_handler_t(fun));
		sqlite3_set_authorizer(handle, sqxx_call_authorize_handler, cb.get());
		setup_callbacks();
		callbacks->authorize_handler = std::move(cb);
	}
	else {
		set_authorize_handler();
	}
}

void connection::set_authorize_handler() {
	sqlite3_set_authorizer(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->authorize_handler.reset();
}

extern "C"
int sqxx_call_busy_handler(void *data, int count) {
	connection::busy_handler_t *fn = reinterpret_cast<connection::busy_handler_t*>(data);
	try {
		return (*fn)(count);
	}
	catch (...) {
		handle_callback_exception("busy handler");
		return 0;
	}
}

void connection::set_busy_handler(const busy_handler_t &fun) {
	if (fun) {
		std::unique_ptr<busy_handler_t> cb(new busy_handler_t(fun));
		sqlite3_busy_handler(handle, sqxx_call_busy_handler, cb.get());
		setup_callbacks();
		callbacks->busy_handler = std::move(cb);
	}
	else {
		set_busy_handler();
	}
}

void connection::set_busy_handler() {
	sqlite3_busy_handler(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->busy_handler.reset();
}

void connection::busy_timeout(int ms) {
	int rv = sqlite3_busy_timeout(handle, ms);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	if (callbacks)
		callbacks->busy_handler.reset();
}


extern "C"
int sqxx_call_progress_handler(void *data) {
	connection::progress_handler_t *fn = reinterpret_cast<connection::progress_handler_t*>(data);
	try {
		return (*fn)();
	}
	catch (...) {
		handle_callback_exception("progress handler");
		return 0;
	}
}

void connection::set_progress_handler(int vinst, const progress_handler_t &fun) {
	if (fun) {
		std::unique_ptr<progress_handler_t> cb(new progress_handler_t(fun));
		sqlite3_progress_handler(handle, vinst, sqxx_call_progress_handler, cb.get());
		setup_callbacks();
		callbacks->progress_handler = std::move(cb);
	}
	else {
		set_progress_handler();
	}
}

void connection::set_progress_handler() {
	sqlite3_progress_handler(handle, 0, nullptr, nullptr);
	if (callbacks)
		callbacks->progress_handler.reset();
}


extern "C"
int sqxx_call_wal_handler(void *data, sqlite3* conn, const char *dbname, int pages) {
	// We registered on a certain connection, and thats the connection we
	// expect to get back here. So `conn` seems to be pretty useless here.
	unused(conn);
	connection::wal_handler_t *fn = reinterpret_cast<connection::wal_handler_t*>(data);
	try {
		(*fn)(dbname, pages);
		return SQLITE_OK;
	}
	catch (...) {
		// TODO: Better exception handling?
		handle_callback_exception("wal handler");
		return SQLITE_ERROR;
	}
}

void connection::set_wal_handler(const wal_handler_t &fun) {
	if (fun) {
		std::unique_ptr<wal_handler_t> cb(new wal_handler_t(fun));
		sqlite3_wal_hook(handle, sqxx_call_wal_handler, cb.get());
		setup_callbacks();
		callbacks->wal_handler = std::move(cb);
	}
	else {
		set_wal_handler();
	}
}

void connection::set_wal_handler() {
	sqlite3_wal_hook(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->wal_handler.reset();
}

void connection::wal_autocheckpoint(int frames) {
	int rv = sqlite3_wal_autocheckpoint(handle, frames);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	if (callbacks)
		callbacks->wal_handler.reset();
}

std::pair<int, int> connection::wal_checkpoint(const char *dbname, int emode) {
	int log, ckpt;
	int rv = sqlite3_wal_checkpoint_v2(handle, dbname, emode, &log, &ckpt);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return std::make_pair(log, ckpt);
}

std::pair<int, int> connection::wal_checkpoint_passive(const char *dbname) {
	return wal_checkpoint(dbname, SQLITE_CHECKPOINT_PASSIVE);
}

std::pair<int, int> connection::wal_checkpoint_passive(const std::string &dbname) {
	return wal_checkpoint_passive(dbname.c_str());
}

std::pair<int, int> connection::wal_checkpoint_full(const char *dbname) {
	return wal_checkpoint(dbname, SQLITE_CHECKPOINT_FULL);
}

std::pair<int, int> connection::wal_checkpoint_full(const std::string &dbname) {
	return wal_checkpoint_full(dbname.c_str());
}

std::pair<int, int> connection::wal_checkpoint_restart(const char *dbname) {
	return wal_checkpoint(dbname, SQLITE_CHECKPOINT_RESTART);
}

std::pair<int, int> connection::wal_checkpoint_restart(const std::string &dbname) {
	return wal_checkpoint_restart(dbname.c_str());
}

extern "C"
void sqxx_call_collation_handler(void *data, sqlite3* conn, int textrep, const char *name) {
	// We registered on a certain connection, and thats the connection we
	// expect to get back here. So `conn` seems to be pretty useless here.
	// 
	// We store a reference to the connection in collation_data_t and then
	// could check here if you got the right connection back
	collation_data_t *dat = reinterpret_cast<collation_data_t*>(data);
	try {
		//if (textrep != SQLITE_UTF8) {
		//	throw error("non-utf8 collation requested");
		//}
		//if (conn != data->c.handle) {
		//	throw error("invalid connection");
		//}

		// We pass the connection here because the callback will need
		// it to register the requested collation function
		(dat->fn)(dat->conn, name);
	}
	catch (...) {
		handle_callback_exception("collation handler");
	}
}

void connection::set_collation_handler(const collation_handler_t &fun) {
	if (fun) {
		std::unique_ptr<collation_data_t> d(new collation_data_t(*this, fun));
		sqlite3_collation_needed(handle, d.get(), sqxx_call_collation_handler);
		setup_callbacks();
		callbacks->collation_data = std::move(d);
	}
	else {
		set_collation_handler();
	}
}

void connection::set_collation_handler() {
	sqlite3_collation_needed(handle, nullptr, nullptr);
	if (callbacks)
		callbacks->collation_data.reset();
}

void connection::remove_function(const char *name, int argc) {
	int rv = sqlite3_create_function_v2(handle, name, argc, SQLITE_UTF8, nullptr,
			nullptr, nullptr, nullptr, nullptr);
	if (rv != SQLITE_OK) {
		throw static_error(rv);
	}
}

void connection::remove_function(const std::string &name, int argc) {
	remove_function(name.c_str(), argc);
}

void connection::remove_aggregate(const char *name, int argc) {
	// Aggregates are really just functions for sqlite
	remove_function(name, argc);
}

void connection::remove_aggregate(const std::string &name, int argc) {
	remove_aggregate(name.c_str(), argc);
}

} // namespace sqxx

