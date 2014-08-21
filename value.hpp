
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_VALUE_HPP_INCLUDED)
#define SQXX_VALUE_HPP_INCLUDED

#include "datatypes.hpp"

typedef struct Mem sqlite3_value;

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
	 * Implicit conversions for the contained value.
	 *
	 * Like `val<>()`.
	 */
	operator int() const;
	operator int64_t() const;
	operator double() const;
	operator const char*() const;
	operator std::string() const;
	operator blob() const;

	/**
	 * Access to the underlying raw `sqlite3_value*`
	 */
	sqlite3_value* raw();
};

} // namespace sqxx

#include "value.impl.hpp"

#endif // SQXX_VALUE_HPP_INCLUDED

