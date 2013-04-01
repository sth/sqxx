
// (c) 2013 Stephan Hohe

#include "sqlitepp.hpp"
#include "sqlitepp_hooks.hpp"
#include "sqlitepp_detail.hpp"
#include <sqlite3.h>
#include <cstring>

#if (SQLITE_VERSION_NUMBER < 3007014)
#include <iostream>
#endif

namespace sqlitepp {

// ---------------------------------------------------------------------------
// error

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
typedef std::function<int (size_t, const char *, size_t, const char *)> collate_stdfun;

int call_collation_compare(void *data, int llen, const void *lstr, int rlen, const void *rstr) {
	collate_stdfun *fun = reinterpret_cast<collate_stdfun*>(data);
	try {
		return (*fun)(
				static_cast<size_t>(llen), reinterpret_cast<const char*>(lstr),
				static_cast<size_t>(rlen), reinterpret_cast<const char*>(rstr)
			);
	}
	catch (...) {
		return 0;
	}
}

void call_collation_destroy(void *data) {
	collate_stdfun *fun = reinterpret_cast<collate_stdfun*>(data);
	delete fun;
}

void connection::create_collation(const char *name, const collate_stdfun &coll) {
	int rv;
	if (coll) {
		collate_stdfun *data = new collate_stdfun(coll);
		rv = sqlite3_create_collation_v2(handle, name, SQLITE_UTF8, data, call_collation_compare, call_collation_destroy);
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

void connection::exec(const char *sql) {
	prepare(sql).exec();
}

statement connection::prepare(const char *sql) {
	int rv;
	sqlite3_stmt *stmt = nullptr;

	rv = sqlite3_prepare_v2(handle, sql, strlen(sql)+1, &stmt, nullptr);
	if (rv != SQLITE_OK) {
		throw static_error(rv);
	}

	return statement(stmt);
}

void connection::interrupt() {
	sqlite3_interrupt(handle);
}

int connection::limit(int id, int newValue) {
	return sqlite3_limit(handle, id, newValue);
}

hook_table& connection::hooks() {
	if (!hook_tab) {
		hook_tab.reset(new hook_table(*this));
	}
	return *hook_tab;
}

void connection::release_memory() {
	sqlite3_db_release_memory(handle);
}


// ---------------------------------------------------------------------------
// statement

statement::statement(sqlite3_stmt *a_handle) : handle(a_handle) {
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

column statement::col(int idx) {
	return column(*this, idx);
}

bool statement::step() {
	int rv;

	rv = sqlite3_step(handle);
	if (rv == SQLITE_ROW) {
		return true;
	}
	else if (rv == SQLITE_DONE) {
		return false;
	}
	else {
		throw static_error(rv);
	}
}

void statement::exec() {
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

parameter::parameter(statement &a_s, int a_idx) : s(a_s), idx(a_idx) {
}

const char* parameter::name() {
	const char *n = sqlite3_bind_parameter_name(s.handle, idx);
	if (!n)
		throw error(SQLITE_RANGE, "cannot determine parameter name");
	return n;
}

void parameter::bind(int value) {
	int rv = sqlite3_bind_int(s.handle, idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(int64_t value) {
	int rv = sqlite3_bind_int64(s.handle, idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(double value) {
	int rv = sqlite3_bind_double(s.handle, idx, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(const char *value, bool copy) {
	int rv;
	if (value == nullptr)
		rv = sqlite3_bind_null(s.handle, idx);
	else
		rv = sqlite3_bind_text(s.handle, idx, value, -1, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind(const blob &value, bool copy) {
	int rv = sqlite3_bind_blob(s.handle, idx, value.first, value.second, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void parameter::bind_zeroblob(int len) {
	int rv = sqlite3_bind_zeroblob(s.handle, idx, len);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

// ---------------------------------------------------------------------------
// column

column::column(statement &a_s, int a_idx) : s(a_s), idx(a_idx) {
}

const char* column::name() const {
	return sqlite3_column_name(s.handle, idx);
}

const char* column::database_name() const {
	return sqlite3_column_database_name(s.handle, idx);
}

const char* column::table_name() const {
	return sqlite3_column_table_name(s.handle, idx);
}

const char* column::origin_name() const {
	return sqlite3_column_origin_name(s.handle, idx);
}

int column::type() const {
	return sqlite3_column_type(s.handle, idx);
}

const char* column::decl_type() const {
	return sqlite3_column_decltype(s.handle, idx);
}


template<>
int column::val<int>() const {
	return sqlite3_column_int(s.handle, idx);
}

template<>
int64_t column::val<int64_t>() const {
	return sqlite3_column_int64(s.handle, idx);
}

template<>
double column::val<double>() const {
	return sqlite3_column_double(s.handle, idx);
}

template<>
const char* column::val<const char*>() const {
	return reinterpret_cast<const char*>(sqlite3_column_text(s.handle, idx));
}

template<>
blob column::val<blob>() const {
	const void* data = sqlite3_column_blob(s.handle, idx);
	int len = sqlite3_column_bytes(s.handle, idx);
	return std::make_pair(data, len);
}

/*
int result::changes() {
	return sqlite3_changes(s.c.handle);
}
*/

struct lib_setup {
	lib_setup() {
		sqlite3_initialize();
	}
	~lib_setup() {
		sqlite3_shutdown();
	}
};

namespace {
	lib_setup setup;
}

} // namespace sqlitepp

