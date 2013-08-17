
// (c) 2013 Stephan Hohe

#include "sqxx.hpp"
#include "detail.hpp"
#include <sqlite3.h>
#include <cstring>
#include <iostream>


namespace sqxx {

// ---------------------------------------------------------------------------
// errors

#if (SQLITE_VERSION_NUMBER >= 3007015)
static_error::static_error(int code_arg) : error(code_arg, sqlite3_errstr(code_arg)) {
}
#else
static_error::static_error(int code_arg) : error(code_arg, "sqlite error " + std::to_string(code_arg)) {
}
#endif

managed_error::managed_error(int code_arg, char *what_arg) : error(code_arg, what_arg), sqlitestr(what_arg) {
}

managed_error::~managed_error() noexcept {
	sqlite3_free(sqlitestr);
}

recent_error::recent_error(sqlite3 *handle) : error(sqlite3_errcode(handle), sqlite3_errmsg(handle)) {
}

// ---------------------------------------------------------------------------
// helper functions

void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept {
	std::cerr << "SQXX: uncaught exeption in " << cbname << ": ";
	if (!ex) {
		std::cerr << "(exception not captured)";
	}
	else {
		try {
			std::rethrow_exception(ex);
		}
		catch (const std::exception &rex) {
			const char *what = rex.what();
			if (what)
				std::cerr << what;
			else
				std::cerr << "(no message)";
		}
		catch (...) {
			std::cerr << "(unknown exception type)";
		}
	}
	std::cerr << std::endl;
}

static callback_exception_handler_t callback_exception_handler = default_callback_exception_handler;

void handle_callback_exception(const char *cbname) {
	if (callback_exception_handler) {
		try {
			callback_exception_handler(cbname, std::current_exception());
		}
		catch (...) {
			default_callback_exception_handler("callback exception handler", std::current_exception());
		}
	}
}

struct collation_data_t {
	connection &c;
	connection::collation_handler_t fn;
	collation_data_t(connection &c_arg, const connection::collation_handler_t &fn_arg)
		: c(c_arg), fn(fn_arg) {
	}
};

namespace detail {

class callback_table {
public:
	std::unique_ptr<connection::commit_handler_t> commit_handler;
	std::unique_ptr<connection::rollback_handler_t> rollback_handler;
	std::unique_ptr<connection::update_handler_t> update_handler;
	std::unique_ptr<connection::trace_handler_t> trace_handler;
	std::unique_ptr<connection::profile_handler_t> profile_handler;
	std::unique_ptr<connection::authorize_handler_t> authorize_handler;
	std::unique_ptr<connection::busy_handler_t> busy_handler;
	std::unique_ptr<collation_data_t> collation_data;
};

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

const char *connection::filename(const char *db) const {
	return sqlite3_db_filename(handle, db);
}

bool connection::readonly(const char *dbname) const {
	int rw = sqlite3_db_readonly(handle, dbname);
	if (rw == -1)
		throw error(SQLITE_ERROR, "no such database");
	return bool(rw);
}

std::pair<int, int> connection::status(int op, bool reset) {
	int rv;
	int cur, hi;
	rv = sqlite3_db_status(handle, op, &cur, &hi, static_cast<int>(reset));
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return std::make_pair(cur, hi);
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
		flags = SQLITE_OPEN_READWRITE;
	rv = sqlite3_open_v2(filename, &handle, flags, nullptr);
	if (rv != SQLITE_OK) {
		if (handle)
			close();
		throw static_error(rv);
	}
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

/* Collation functions.
 * 
 * Strings passed to collation callback functions are not null-terminated.
 */
extern "C"
int sqxx_call_collation_compare(void *data, int llen, const void *lstr, int rlen, const void *rstr) {
	typedef connection::collation_function_t cf_t;
	cf_t *fun = reinterpret_cast<cf_t*>(data);
	try {
		return (*fun)(
				llen, reinterpret_cast<const char*>(lstr),
				rlen, reinterpret_cast<const char*>(rstr)
			);
	}
	catch (...) {
		handle_callback_exception("collation function");
		return 0;
	}
}

extern "C"
void sqxx_call_collation_destroy(void *data) {
	typedef connection::collation_function_t cf_t;
	cf_t *fun = reinterpret_cast<cf_t*>(data);
	delete fun;
}

void connection::create_collation(const char *name, const connection::collation_function_t &coll) {
	typedef connection::collation_function_t cf_t;
	int rv;
	if (coll) {
		cf_t *data = new cf_t(coll);
		rv = sqlite3_create_collation_v2(handle, name, SQLITE_UTF8, data, sqxx_call_collation_compare, sqxx_call_collation_destroy);
		if (rv != SQLITE_OK) {
			delete data;
			throw static_error(rv);
		}
	}
	else {
		rv = sqlite3_create_collation_v2(handle, name, SQLITE_UTF8, nullptr, nullptr, nullptr);
		if (rv != SQLITE_OK)
			throw static_error(rv);
	}
}

/*
struct exec_context {
	connection *con;
	const std::function<bool ()> &callback;
	std::exception_ptr ex;
};

int exec_callback(void *data, int, char**, char**) {
	exec_context *ctx = reinterpret_cast<exec_context*>(data);
	try {
		return ctx->fn();
	}
	catch (...) {
		ctx->ex = std::current_exception();
		return 1;
	}
}

void connection::exec(const char *sql, const std::function<bool ()> &callback) {
	char *errmsg = nullptr;
	int rv;
	if (callback) {
		exec_context ctx = {
			this,
			callback
		};
		rv = sqlite3_exec(handle, sql, exec_callback, &ctx, &errmsg);
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
		throw error(rv);
	}
}
*/

statement connection::exec(const char *sql) {
	statement st = prepare(sql);
	st.run();
	return st;
}

statement connection::exec(const std::string &sql) {
	return exec(sql.c_str());
}

statement connection::prepare(const char *sql) {
	int rv;
	sqlite3_stmt *stmt = nullptr;

	rv = sqlite3_prepare_v2(handle, sql, strlen(sql)+1, &stmt, nullptr);
	if (rv != SQLITE_OK) {
		throw static_error(rv);
	}

	return statement(*this, stmt);
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
		callbacks.reset(new detail::callback_table);
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
void sqxx_call_collation_handler(void *data, sqlite3 *handle, int textrep, const char *name) {
	collation_data_t *dat = reinterpret_cast<collation_data_t*>(data);
	//assert(handle == dat->c.handle);
	try {
		(dat->fn)(dat->c, name);
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


// ---------------------------------------------------------------------------
// statement

statement::statement(connection &conn_arg, sqlite3_stmt *handle_arg)
		: handle(handle_arg), conn(conn_arg), completed(true) {
}

statement::~statement() {
	if (handle) {
		sqlite3_finalize(handle);
	}
}

const char* statement::sql() {
	return sqlite3_sql(handle);
}

int statement::status(int op, bool reset) {
	return sqlite3_stmt_status(handle, op, static_cast<int>(reset));
}

int statement::param_index(const char *name) const {
	int idx = sqlite3_bind_parameter_index(handle, name);
	if (idx == 0)
		throw error(SQLITE_RANGE, std::string("cannot find parameter \"") + name + "\"");
	return idx;
}

int statement::param_count() const {
	return sqlite3_bind_parameter_count(handle);
}

parameter statement::param(int idx) {
	return parameter(*this, idx);
}

parameter statement::param(const char *name) {
	return parameter(*this, param_index(name));
}

int statement::col_count() const {
	return sqlite3_column_count(handle);
}

column<void> statement::col(int idx) {
	return column<void>(*this, idx);
}

void statement::step() {
	int rv;

	rv = sqlite3_step(handle);
	if (rv == SQLITE_ROW) {
		completed = false;
	}
	else if (rv == SQLITE_DONE) {
		completed = true;
	}
	else {
		throw static_error(rv);
	}
}

void statement::run() {
	// TODO: Check that statement was not yet run(), maybe reset() if it was
	step();
}

void statement::next_row() {
	// TODO: Add check that stmt was run()
	step();
}

void statement::reset() {
	int rv;

	rv = sqlite3_reset(handle);
	if (rv != SQLITE_OK) {
		// last step() gave error. Does this mean reset() is also invalid?
		// TODO
	}
}

void statement::clear_bindings() {
	int rv;

	rv = sqlite3_clear_bindings(handle);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

// ---------------------------------------------------------------------------
// parameter

parameter::parameter(statement &a_stmt, int a_idx) : stmt(a_stmt), idx(a_idx) {
}

const char* parameter::name() const {
	const char *n = sqlite3_bind_parameter_name(stmt.raw(), idx);
	if (!n)
		throw error(SQLITE_RANGE, "cannot determine parameter name");
	return n;
}

void parameter::bind(int value) {
	int rv = sqlite3_bind_int(stmt.raw(), idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(int64_t value) {
	int rv = sqlite3_bind_int64(stmt.raw(), idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(double value) {
	int rv = sqlite3_bind_double(stmt.raw(), idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(const char *value, bool copy) {
	int rv;
	if (value == nullptr)
		rv = sqlite3_bind_null(stmt.raw(), idx);
	else
		rv = sqlite3_bind_text(stmt.raw(), idx, value, -1, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(const blob &value, bool copy) {
	int rv = sqlite3_bind_blob(stmt.raw(), idx, value.first, value.second, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind_zeroblob(int len) {
	int rv = sqlite3_bind_zeroblob(stmt.raw(), idx, len);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

// ---------------------------------------------------------------------------
// column

column_base::column_base(statement &a_stmt, int a_idx) : stmt(a_stmt), idx(a_idx) {
}

const char* column_base::name() const {
	return sqlite3_column_name(stmt.raw(), idx);
}

const char* column_base::database_name() const {
	return sqlite3_column_database_name(stmt.raw(), idx);
}

const char* column_base::table_name() const {
	return sqlite3_column_table_name(stmt.raw(), idx);
}

const char* column_base::origin_name() const {
	return sqlite3_column_origin_name(stmt.raw(), idx);
}

int column_base::type() const {
	return sqlite3_column_type(stmt.raw(), idx);
}

const char* column_base::decl_type() const {
	return sqlite3_column_decltype(stmt.raw(), idx);
}

template<>
int column_base::val<int>() const {
	return sqlite3_column_int(stmt.raw(), idx);
}

template<>
int64_t column_base::val<int64_t>() const {
	return sqlite3_column_int64(stmt.raw(), idx);
}

template<>
double column_base::val<double>() const {
	return sqlite3_column_double(stmt.raw(), idx);
}

template<>
const char* column_base::val<const char*>() const {
	return reinterpret_cast<const char*>(sqlite3_column_text(stmt.raw(), idx));
}

template<>
blob column_base::val<blob>() const {
	// Correct order to call functions according to http://www.sqlite.org/c3ref/column_blob.html
	const void *data = sqlite3_column_blob(stmt.raw(), idx);
	int len = sqlite3_column_bytes(stmt.raw(), idx);
	return std::make_pair(data, len);
}


statement::row_iterator::row_iterator(statement *s_arg) : s(s_arg) {
	check_complete();
}

void statement::row_iterator::check_complete() {
	if (s && s->completed)
		s = nullptr;
}

statement::row_iterator& statement::row_iterator::operator++() {
	s->step();
	check_complete();
	return *this;
}

int statement::changes() const {
	return sqlite3_changes(conn.raw());
}


struct lib_setup {
	lib_setup() {
		sqlite3_initialize();
	}
	~lib_setup() {
		sqlite3_shutdown();
	}
};


std::pair<int, int> status(int op, bool reset) {
	int rv;
	int cur, hi;
	rv = sqlite3_status(op, &cur, &hi, static_cast<int>(reset));
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return std::make_pair(cur, hi);
}

namespace {
	lib_setup setup;
}

} // namespace sqxx

