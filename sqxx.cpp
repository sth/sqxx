
// (c) 2013 Stephan Hohe

#include "sqxx.hpp"
#include "error.hpp"
#include <sqlite3.h>
#include <cstring>
#include <iostream>

namespace sqxx {

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

} // namespace sqxx

