
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_CONTEXT_HPP_INCLUDED)
#define SQXX_CONTEXT_HPP_INCLUDED

#include "datatypes.hpp"

struct sqlite3_context;

namespace sqxx {

// Aggregate function interoperability
namespace detail {
	class aggregate_data;
}

/** Wraps `struct sqlite3_context` */
class context {
private:
	// UNOWNED
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
	void result();

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
	 * Sets result to error SQLITE_MISUSE
	 *
	 * Wraps [`sqlite3_result_error_code(SQLITE_MISUSE)`](http://www.sqlite.org/c3ref/result_blob.html)
	 */
	void result_error_misuse();

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
	if_selected_type<R, void, int, int64_t, double>
	result(R value);

	template<typename R>
	if_selected_type<R, void, const char*>
	result(R value, bool copy=true);

	template<typename R>
	if_selected_type<R, void, std::string, blob>
	result(const R &value, bool copy=true);

	/*
	template<typename R>
	void result(std::optional<R> value);
	*/

private:
	// This is used internally by our aggregate functions
	friend class detail::aggregate_data;
	void* aggregate_context(int bytes);

public:
	/**
	 * Returns a raw pointer to the underlying `struct sqlite3_context`.
	 */
	sqlite3_context* raw();
};

template<>
void context::result<int>(int value);
template<>
void context::result<int64_t>(int64_t value);
template<>
void context::result<double>(double value);
template<>
void context::result(const char *value, bool copy);
template<>
void context::result(const std::string &value, bool copy);
template<>
void context::result(const blob &value, bool copy);

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

