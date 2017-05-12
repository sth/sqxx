
// (c) 2013 Stephan Hohe

#include "statement.hpp"
#include "column.hpp"
#include "parameter.hpp"
#include "error.hpp"
#include <sqlite3.h>

namespace sqxx {

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

int statement::status_fullscan_step(bool reset) {
	return status(SQLITE_STMTSTATUS_FULLSCAN_STEP, reset);
}

int statement::status_sort(bool reset) {
	return status(SQLITE_STMTSTATUS_SORT, reset);
}

int statement::status_autoindex(bool reset) {
	return status(SQLITE_STMTSTATUS_AUTOINDEX, reset);
}

#if SQLITE_VERSION_NUMBER >= 3008000
int statement::status_vm_step(bool reset) {
	return status(SQLITE_STMTSTATUS_VM_STEP, reset);
}
#endif

bool statement::readonly() const {
	return sqlite3_stmt_readonly(handle);
}

bool statement::busy() const {
	return sqlite3_stmt_busy(handle);
}

int statement::param_index(const char *name) const {
	int idx = sqlite3_bind_parameter_index(handle, name);
	if (idx == 0)
		throw error(SQLITE_RANGE, std::string("cannot find parameter \"") + name + "\"");
	return idx-1;
}

int statement::param_index(const std::string &name) const {
	return param_index(name.c_str());
}

int statement::param_count() const {
	return sqlite3_bind_parameter_count(handle);
}

parameter statement::param(int idx) {
	return parameter(*this, idx);
}

parameter statement::param(const char *name) {
	return param(param_index(name));
}

parameter statement::param(const std::string &name) {
	return param(name.c_str());
}

void statement::bind(int idx) {
	int rv = sqlite3_bind_null(handle, idx+1);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

template<>
void statement::bind<int>(int idx, int value) {
	int rv = sqlite3_bind_int(handle, idx+1, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

template<>
void statement::bind<int64_t>(int idx, int64_t value) {
	int rv = sqlite3_bind_int64(handle, idx+1, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

template<>
void statement::bind<double>(int idx, double value) {
	int rv = sqlite3_bind_double(handle, idx+1, value);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

template<>
void statement::bind<const char*>(int idx, const char *value, bool copy) {
	if (value) {
		int rv = sqlite3_bind_text(handle, idx+1, value, -1, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
		if (rv != SQLITE_OK)
			throw static_error(rv);
	}
	else {
		bind(idx);
	}
}

template<>
void statement::bind<std::string>(int idx, const std::string &value, bool copy) {
	int rv = sqlite3_bind_text(handle, idx+1, value.c_str(), value.length(), (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

template<>
void statement::bind<blob>(int idx, const blob &value, bool copy) {
	if (value.data) {
		int rv = sqlite3_bind_blob(handle, idx+1, value.data, value.length, (copy ? SQLITE_TRANSIENT : SQLITE_STATIC));
		if (rv != SQLITE_OK)
			throw static_error(rv);
	}
	else {
		int rv = sqlite3_bind_zeroblob(handle, idx+1, value.length);
		if (rv != SQLITE_OK)
			throw static_error(rv);
	}
}


int statement::col_count() const {
	return sqlite3_column_count(handle);
}

int statement::col_index(const char *name) const {
	if (!col_index_table_built) {
		for (int i = 0, n = col_count(); i < n; ++i) {
			col_index_table[col(i).name()] = i;
		}
		col_index_table_built = true;
	}
	return col_index_table[name];
}

int statement::col_index(const std::string &name) const {
	return col_index(name.c_str());
}

column statement::col(int idx) const {
	return column(*this, idx);
}

column statement::col(const char *name) const {
	return col(col_index(name));
}

column statement::col(const std::string &name) const {
	return col(name.c_str());
}

template<>
int statement::val<int>(int idx) const {
	return sqlite3_column_int(handle, idx);
}

template<>
int64_t statement::val<int64_t>(int idx) const {
	return sqlite3_column_int64(handle, idx);
}

template<>
double statement::val<double>(int idx) const {
	return sqlite3_column_double(handle, idx);
}

template<>
const char* statement::val<const char*>(int idx) const {
	const unsigned char *text = sqlite3_column_text(handle, idx);
	return reinterpret_cast<const char*>(text);
}

template<>
std::string statement::val<std::string>(int idx) const {
	// Correct order to call functions according to http://www.sqlite.org/c3ref/column_blob.html
	const unsigned char* text = sqlite3_column_text(handle, idx);
	int bytes = sqlite3_column_bytes(handle, idx);
	return std::string(reinterpret_cast<const char*>(text), bytes);
}

template<>
blob statement::val<blob>(int idx) const {
	// Correct order to call functions according to http://www.sqlite.org/c3ref/column_blob.html
	const void *data = sqlite3_column_blob(handle, idx);
	int bytes = sqlite3_column_bytes(handle, idx);
	return blob(data, bytes);
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

void statement::query() {
	run();
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

	col_index_table.clear();
	col_index_table_built = false;
}

void statement::clear_bindings() {
	int rv;

	rv = sqlite3_clear_bindings(handle);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

statement::row_iterator::row_iterator(statement *s_arg) : s(s_arg), rowidx(0) {
	check_complete();
}

void statement::row_iterator::check_complete() {
	if (s && s->completed) {
		s = nullptr;
	}
}

statement::row_iterator& statement::row_iterator::operator++() {
	s->step();
	check_complete();
	++rowidx;
	return *this;
}

} // namespace sqxx

