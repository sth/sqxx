
namespace sqxx {

/**
 * Attempt to free heap memory
 *
 * Wraps [`sqlite3_release_memory()`]()
 */
int release_memory(int amount);

/**
 * Imposes a limit on heap size
 *
 * Wraps [`sqlite3_soft_heap_limit()`](soft_heap_limit64.html)
 */
int64_t soft_heap_limit(int64_t limit);

/**
 * Wraps [`sqlite3_sourceid()`](http://www.sqlite.org/c3ref/libversion.html)
 */
const char* c_source_id();

/**
 * Wraps [`sqlite3_libversion()`](http://www.sqlite.org/c3ref/libversion.html)
 */
const char* c_libversion();

/**
 * Wraps [`sqlite3_libversion_number()`](http://www.sqlite.org/c3ref/libversion.html)
 */
int c_libversion_number();

/**
 * Query Sqlite runtime status
 *
 * Wraps [`sqlite3_status()`](http://www.sqlite.org/c3ref/status.html)
 */
int status(int op, int *current, int *highwater, bool reset);

/**
 * Test if the C library was compiler threadsafe.
 *
 * threadsafe();
 */
bool threadsafe();

/**
 * Check if a sqlite3 C library compilation option was used.
 *
 * If the underlying C library was compiled with `SQLITE_OMIT_COMPILEOPTION_DIAGS`,
 * calling this function will lead to linker errors.
 *
 * Wraps [`sqlite3_status()`](http://www.sqlite.org/c3ref/status.html)
 */
int compileoption_used(const char *name);
int compileoption_used(const std::string &name) {
	compileoption_used(name.c_str());
}

/**
 * Check which sqlite3 C library compilation options were used.
 *
 * If the underlying C library was compiled with `SQLITE_OMIT_COMPILEOPTION_DIAGS`,
 * calling this function will lead to linker errors.
 *
 * Wraps [`sqlite3_status()`](http://www.sqlite.org/c3ref/status.html)
 */
const char* compileoption_get(int nr);

/**
 * Determines if a statement is complete.
 *
 * Wraps [`sqlite3_complete()`](http://www.sqlite.org/c3ref/complete.html)
 */
bool complete(const char *sql);
bool complete(const std::string &sql) {
	return complete(sql.c_str());
}

/**
 * Memory allocator statistics
 *
 * Wraps [`sqlite3_memory_used()`](http://www.sqlite.org/c3ref/memory_highwater.html)
 */
uint64_t memory_used();

/**
 * Memory allocator statistics
 *
 * Wraps [`sqlite3_memory_highwater()`](http://www.sqlite.org/c3ref/memory_highwater.html)
 */
uint64_t memory_highwater(bool reset=false);

/**
 * Pseuso-random number generator
 *
 * Wraps [`sqlite3_randomness()`](http://www.sqlite.org/c3ref/randomness.html)
 */
void randomness(int size, void *buf);

} // namespace sqxx

