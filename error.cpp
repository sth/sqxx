
// (c) 2014 Stephan Hohe

#include "error.hpp"
#include <sqlite3.h>
#include <iostream>

namespace sqxx {

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


static callback_exception_handler_t callback_exception_handler = default_callback_exception_handler;

void set_callback_exception_handler(const callback_exception_handler_t &fun) {
	callback_exception_handler = fun;
}

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

} // namespace sqxx

