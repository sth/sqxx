
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_STATEMENT_IMPL_HPP_INCLUDED)
#define SQXX_STATEMENT_IMPL_HPP_INCLUDED

#if !defined(SQXX_STATEMENT_HPP_INCLUDED)
#error "Don't include statement.impl.hpp directly, include statement.hpp instead"
#endif

namespace sqxx {

/** Set a parameter to an int value
 *
 * sqlite3_bind_int()
 */
template<>
void statement::bind<int>(int idx, int value);

/** Set a parameter to an int64_t value
 *
 * sqlite3_bind_int64()
 */
template<>
void statement::bind<int64_t>(int idx, int64_t value);

/** Set a parameter to a double value
 *
 * sqlite3_bind_double()
 */
template<>
void statement::bind<double>(int idx, double value);

/** Set a parameter to a const char* value.
 *
 * With copy=false, tells sqlite to not make an internal copy of the value.
 * This means that the value passed in has to stay available until
 * the query is completed.
 *
 * If `value` is a null pointer, this binds a NULL.
 *
 * sqlite3_bind_text()
 */
template<>
void statement::bind<const char*>(int idx, const char* value, bool copy);

template<>
void statement::bind<std::string>(int idx, const std::string &value, bool copy);

/** Set a parameter to a blob.
 *
 * If the data pointer of the blob is a null pointer,
 * it creates a blob consisting of zeroes.
 *
 * sqlite3_bind_blob()
 * sqlite3_bind_zeroblob()
 */
template<>
void statement::bind<blob>(int idx, const blob &value, bool copy);


/** Wraps [`sqlite3_column_int()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
int statement::val<int>(int idx) const;

/** Wraps [`sqlite3_column_int64()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
int64_t statement::val<int64_t>(int idx) const;

/** Wraps [`sqlite3_column_double()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
double statement::val<double>(int idx) const;

/** Wraps [`sqlite3_column_text()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
const char* statement::val<const char*>(int idx) const;

/** Wraps [`sqlite3_column_text()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
std::string statement::val<std::string>(int idx) const;

/** Wraps [`sqlite3_column_blob()`](http://www.sqlite.org/c3ref/column_blob.html) */
template<>
blob statement::val<blob>(int idx) const;

template<typename T>
if_sqxx_db_type<T, T> statement::val(const char *name) const {
	return val<T>(col_index(name));
}

template<typename T>
if_sqxx_db_type<T, T> statement::val(const std::string &name) const {
	return val<T>(name).c_str();
}

} // namespace sqxx

#endif // SQXX_STATEMENT_IMPL_HPP_INCLUDED

