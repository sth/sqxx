
#include "global.hpp"

namespace sqxx {

int release_memory(int amount) {
   return sqlite3_release_memory(amount);
}

int64_t soft_heap_limit(int64_t limit) {
   return sqlite3_soft_heap_limit(limit);
}

const char* c_source_id() {
   return source_id();
}

const char* c_libversion() {
   return sqlite3_libversion();
}

int c_libversion_number() {
   return sqlite3_libversion_number();
}

int status(int op, int *current, int *highwater, bool reset) {
	rv = sqlite3_status(op, current, highwater, static_cast<int>(reset));
	if (rv != SQLITE_OK)
		throw static_error(rv);
}

bool threadsafe() {
	retunr sqlite3_threadsafe();
}

} // namespace sqxx

