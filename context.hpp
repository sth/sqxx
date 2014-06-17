
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_CONTEXT_HPP_INCLUDED)
#define SQXX_CONTEXT_HPP_INCLUDED

#include "datatypes.hpp"

struct sqlite3_context;

namespace sqxx {

/** Wraps `struct sqlite3_context` */
class context {
private:
	sqlite3_context *handle;

public:
	context(sqlite3_context *handle_arg) : handle(handle_arg) {
	}

	/**
	 * Sets result to NULL:
	 *
	 * Wraps [`sqlite3_result_null()`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_null();
	/**
	 * Sets result to error SQLITE_MISUSE
	 *
	 * Wraps [`sqlite3_result_error_code(SQLITE_MISUSE)`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_misuse();
	/**
	 * Sets result to error with the given message
	 *
	 * Wraps [`sqlite3_result_error()`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_error(const char *msg);
	/**
	 * Sets result to error with the given error code
	 *
	 * Wraps [`sqlite3_result_error_code()`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_error_code(int code);
	/**
	 * Sets result to error "out of memory"
	 *
	 * Wraps [`sqlite3_result_error_nomem()`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_error_nomem();
	/**
	 * Sets result to error "too big"
	 *
	 * Wraps [`sqlite3_result_error_toobig()`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_error_toobig();

	/**
	 * Sets result to given value
	 *
	 * Wraps
	 * [`sqlite3_result_int()`](http://www.sqlite.org/c3ref/result_blob.html),
	 * [`sqlite3_result_int64()`](http://www.sqlite.org/c3ref/result_blob.html),
	 * [`sqlite3_result_double()`](http://www.sqlite.org/c3ref/result_blob.html),
	 * [`sqlite3_result_text()`](http://www.sqlite.org/c3ref/result_blob.html),
	 * [`sqlite3_result_blob()`](http://www.sqlite.org/c3ref/result_blob.html),
	 * and [`sqlite3_result_zeroblob()`](http://www.sqlite.org/c3ref/result_blob.html),
	 */
	template<typename R>
	if_sqxx_db_type<R, void>
	result(R value);

	/*
	template<typename R>
	void result(std::optional<R> value);
	*/

	/**
	 * Returns a raw pointer to the underlying `struct sqlite3_context`.
	 */
	sqlite3_context* raw();
};

/** sqlite3_result_int() */
template<>
void context::result<int>(int value);

// TODO: More specializations

/*
template<>
class result<std::string> : public result<const char*> {
	typedef result<const char*> base_type;

	result(const std::string &value, bool copy=true) : base_type(value.c_string, copy) {
	}

	void apply(sqlite3_context *ctx);
};
*/

} // namespace sqxx

#endif // SQXX_CONTEXT_HPP_INCLUDED

