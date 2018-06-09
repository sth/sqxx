
#include "config.hpp"
#include "error.hpp"
#include <memory>
#include <sqlite3.h>

namespace sqxx {

namespace detail {
	struct config_callback_table {
		std::unique_ptr<log_handler_t> log_handler;
		std::unique_ptr<sqllog_handler_t> sqllog_handler;
	};
}

detail::config_callback_table config_callbacks;


void config_singlethread() {
	int rv = sqlite3_config(SQLITE_CONFIG_SINGLETHREAD);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_multithread() {
	int rv = sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_serialized() {
	int rv = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_malloc(mem_methods*);
void config_getmalloc(mem_methods*);

void config_memstatus(bool enable) {
	int rv = sqlite3_config(SQLITE_CONFIG_MEMSTATUS, enable);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_scratch(void *buf, int sz, int n) {
	int rv = sqlite3_config(SQLITE_CONFIG_SCRATCH, buf, sz, n);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_pagecache(void *buf, int sz, int n) {
	int rv = sqlite3_config(SQLITE_CONFIG_PAGECACHE, buf, sz, n);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_heap(void *buf, int bytes, int minalloc) {
	int rv = sqlite3_config(SQLITE_CONFIG_HEAP, buf, bytes, minalloc);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_mutex(mutex_methods*);
void config_getmutex(mutex_methods*);

void config_lookaside(int sz, int n) {
	int rv = sqlite3_config(SQLITE_CONFIG_LOOKASIDE, sz, n);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_pcache2(pcache_methods2*);
void config_getpcache2(pcache_methods2*);

extern "C"
void sqxx_call_config_log_handler(void *data, int err, const char *msg) {
	log_handler_t *fn = reinterpret_cast<log_handler_t*>(data);
	try {
		//(*fn)(static_error(err), msg);
		(*fn)(err, msg);
	}
	catch (...) {
		handle_callback_exception("log handler");
	}
}

void config_log(const log_handler_t &fun) {
	if (fun) {
		std::unique_ptr<log_handler_t> cb = std::make_unique<log_handler_t>(fun);
		int rv = sqlite3_config(SQLITE_CONFIG_LOG, sqxx_call_config_log_handler, cb.get());
		if (rv != SQLITE_OK)
			throw static_error(rv);
		config_callbacks.log_handler = std::move(cb);
	}
	else {
		config_log();
	}
}

void config_log() {
	int rv = sqlite3_config(SQLITE_CONFIG_LOG, nullptr, nullptr);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_uri(bool enable) {
	int rv = sqlite3_config(SQLITE_CONFIG_URI, enable);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_covering_index_scan(bool enable) {
	int rv = sqlite3_config(SQLITE_CONFIG_COVERING_INDEX_SCAN, enable);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

extern "C"
void sqxx_call_config_sqllog_handler(void *data, sqlite3* /*conn*/, const char *msg, int kind) {
	// TODO: `conn` might be useful to have here, but we'd need a
	// non-owning version.
	sqllog_handler_t *fn = reinterpret_cast<sqllog_handler_t*>(data);
	try {
		(*fn)(/*conn,*/ msg, kind);
	}
	catch (...) {
		handle_callback_exception("sqllog handler");
	}
}

void config_sqllog(const sqllog_handler_t &fun) {
	if (fun) {
		std::unique_ptr<sqllog_handler_t> cb = std::make_unique<sqllog_handler_t>(fun);
		int rv = sqlite3_config(SQLITE_CONFIG_SQLLOG, sqxx_call_config_sqllog_handler,
				cb.get());
		if (rv != SQLITE_OK)
			throw static_error(rv);
		config_callbacks.sqllog_handler = std::move(cb);
	}
	else {
		config_sqllog();
	}
}

void config_sqllog() {
	int rv = sqlite3_config(SQLITE_CONFIG_SQLLOG, nullptr, nullptr);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

void config_mmap_size(int64_t defaultlimit, int64_t maxlimit) {
	int rv = sqlite3_config(SQLITE_CONFIG_MMAP_SIZE, defaultlimit, maxlimit);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

#if defined(SQLITE_WIN32_MALLOC)
void config_win32_heapsize(uint32_t maxsize) {
	int rv = sqlite3_config(SQLITE_CONFIG_WIN32_HEAPSIZE, maxsize);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}
#endif

#if SQLITE_VERSION_NUMBER >= 3008008
int config_pcache_hdrsz() {
	int result;
	int rv = sqlite3_config(SQLITE_CONFIG_PCACHE_HDRSZ, &result);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return result;
}
#endif

#if SQLITE_VERSION_NUMBER >= 3008008
void config_pmasz(unsigned int minsize) {
	int rv = sqlite3_config(SQLITE_CONFIG_PMASZ, minsize);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}
#endif

#if SQLITE_VERSION_NUMBER >= 3012000
void config_stmtjrnl_spill(int threshold) {
	int rv = sqlite3_config(SQLITE_CONFIG_PMASZ, threshold);
	if (rv != SQLITE_OK)
		throw static_error(rv);
}
#endif

} // namespace sqxx

