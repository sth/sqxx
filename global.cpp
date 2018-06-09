
#include "global.hpp"
#include "error.hpp"
#include <sqlite3.h>

namespace sqxx {

int release_memory(int amount) {
   return sqlite3_release_memory(amount);
}

int64_t soft_heap_limit(int64_t limit) {
   return sqlite3_soft_heap_limit64(limit);
}

const char* capi_source_id() {
   return sqlite3_sourceid();
}

const char* capi_libversion() {
   return sqlite3_libversion();
}

int capi_libversion_number() {
   return sqlite3_libversion_number();
}

counter status(int op, bool reset) {
#if SQLITE_VERSION_NUMBER >= 3008009
	tcounter<sqlite3_int64> result;
	int rv = sqlite3_status64(op, &result.current, &result.highwater, static_cast<int>(reset));
#else
	tcounter<int> result;
	int rv = sqlite3_status(op, &result.current, &result.highwater, static_cast<int>(reset));
#endif
	if (rv != SQLITE_OK)
		throw static_error(rv);
	return result;
}

counter status_memory_used(bool reset) {
	return status(SQLITE_STATUS_MEMORY_USED, reset);
}

counter status_malloc_size(bool reset) {
	return status(SQLITE_STATUS_MALLOC_SIZE, reset);
}

counter status_malloc_count(bool reset) {
	return status(SQLITE_STATUS_MALLOC_COUNT, reset);
}

counter status_pagecache_used(bool reset) {
	return status(SQLITE_STATUS_PAGECACHE_USED, reset);
}

counter status_pagecache_overflow(bool reset) {
	return status(SQLITE_STATUS_PAGECACHE_OVERFLOW, reset);
}

counter status_pagecache_size(bool reset) {
	return status(SQLITE_STATUS_PAGECACHE_SIZE, reset);
}

counter status_scratch_used(bool reset) {
	return status(SQLITE_STATUS_SCRATCH_USED, reset);
}

counter status_scratch_overflow(bool reset) {
	return status(SQLITE_STATUS_SCRATCH_OVERFLOW, reset);
}

counter status_scratch_size(bool reset) {
	return status(SQLITE_STATUS_SCRATCH_SIZE, reset);
}

counter status_parser_stack(bool reset) {
	return status(SQLITE_STATUS_PARSER_STACK, reset);
}

bool threadsafe() {
	return sqlite3_threadsafe();
}

int compileoption_used(const char *name) {
	return sqlite3_compileoption_used(name);
}

int compileoption_used(const std::string &name) {
	return compileoption_used(name.c_str());
}

const char* compileoption_get(int nr) {
	return sqlite3_compileoption_get(nr);
}

bool complete(const char *sql) {
	return sqlite3_complete(sql);
}

bool complete(const std::string &sql) {
	return complete(sql.c_str());
}

int64_t memory_used() {
	return sqlite3_memory_used();
}

int64_t memory_highwater(bool reset) {
	return sqlite3_memory_highwater(reset);
}

void randomness(int size, void *buf) {
	sqlite3_randomness(size, buf);
}

} // namespace sqxx

