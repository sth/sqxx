
// (c) 2013 Stephan Hohe

#if !defined(SQXX_FUNC_HPP_INCLUDED)
#define SQXX_FUNC_HPP_INCLUDED

#include "datatypes.hpp"
#include <functional>
#include "sqxx.hpp"

typedef struct Mem sqlite3_value;
struct sqlite3_context;

namespace sqxx {

/** struct sqlite3_value */
class value {
private:
	sqlite3_value *handle;
public:
	value(sqlite3_value *handle_arg);

	/**
	 * Checks if the value is NULL
	 */
	bool null() const;

	/**
	 * Access the value as a certain type
	 *
	 * Wraps [`sqlite3_value_*()`](http://www.sqlite.org/c3ref/value_blob.html)
	 */
	template<typename T>
	if_sqxx_db_type<T, T>
	val() const;

	/**
	 * Implicit vonversions for the contained value.
	 *
	 * Like `val<>()`.
	 */
	operator int() const;
	operator int64_t() const;
	operator double() const;
	operator const char*() const;
	operator blob() const;

	/**
	 * Access to the underlying raw `sqlite3_value*`
	 */
	sqlite3_value* raw();
};

/** sqlite3_value_int() */
template<>
int value::val<int>() const;

/** sqlite3_value_int64() */
template<>
int64_t value::val<int64_t>() const;

/** sqlite3_value_double() */
template<>
double value::val<double>() const;

/** sqlite3_value_text() */
template<>
const char* value::val<const char*>() const;

/** sqlite3_value_blob() */
template<>
blob value::val<blob>() const;


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

namespace detail {

// count number of types

template<typename... Ts>
struct type_count;

template<>
struct type_count<> {
	static const size_t value = 0;
};

template<typename T1, typename... Ts>
struct type_count<T1, Ts...> {
	static const size_t value = 1 + type_count<Ts...>::value;
};


// Apply an array of sqlite3_value's to a function

template<int I, int N, typename R, typename Fun>
struct apply_value_array_st {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value **argv, Values... args) {
		return apply_value_array_st<I+1, N, R, Fun>::template apply<Values..., value>(
				std::forward<Fun>(f),
				argv,
				std::forward<Values>(args)..., 
				value(argv[I])
			);
	}
};

template<int I, typename R, typename Fun>
struct apply_value_array_st<I, I, R, Fun> {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value **argv, Values... args) {
		return f(std::forward<Values>(args)...);
	}
};

template<int N, typename R, typename Fun>
R apply_value_array(Fun f, sqlite3_value **argv) {
	return apply_value_array_st<0, N, R, Fun>::apply(f, argv);
}

struct function_data {
	virtual ~function_data() {
	}
	virtual void call(context &ctx, int argc, sqlite3_value **argv) = 0;
};

template<size_t NArgs, typename R, typename... Ps>
struct function_data_t : function_data {
	std::function<R (Ps...)> fun;

	function_data_t(const std::function<R (Ps...)> &fun_arg) : fun(fun_arg) {
	}

	virtual void call(context &ctx, int argc, sqlite3_value **argv) override {
		if (argc != NArgs) {
			ctx.result_misuse();
		}
		else {
			ctx.result<R>(apply_value_array<NArgs, R>(fun, argv));
		}
	}
};

template<size_t NArgs, typename Fun, typename R, typename... Ps>
struct function_data_gen_t : function_data {
	Fun fun;

	function_data_gen_t(Fun fun_arg) : fun(std::forward<Fun>(fun_arg)) {
	}

	virtual void call(context &ctx, int argc, sqlite3_value **argv) override {
		if (argc != NArgs) {
			ctx.result_misuse();
		}
		else {
			ctx.result<R>(apply_value_array<NArgs, R>(fun, argv));
		}
	}
};


} // namespace detail

template<typename R, typename... Ps>
void connection::create_function(const char *name, const std::function<R (Ps...)> &fun) {
	static const size_t NArgs = detail::type_count<Ps...>::value;
	create_function_p(name, NArgs, new detail::function_data_t<NArgs, R, Ps...>(fun));
}

} // namespace sqxx

#endif // SQXX_FUNC_HPP_INCLUDED

