
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

} // namespace sqxx

