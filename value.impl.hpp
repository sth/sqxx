
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_VALUE_IMPL_HPP_INCLUDED)
#define SQXX_VALUE_IMPL_HPP_INCLUDED

#if !defined(SQXX_VALUE_HPP_INCLUDED)
#error "Don't include value.impl.hpp directly, include value.hpp instead"
#endif

namespace sqxx {

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
template<>
std::string value::val<std::string>() const;

/** sqlite3_value_blob() */
template<>
blob value::val<blob>() const;

} // namespace sqxx

#endif // SQXX_VALUE_IMPL_HPP_INCLUDED

