
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_COLUMN_IMPL_HPP_INCLUDED)
#define SQXX_COLUMN_IMPL_HPP_INCLUDED

#if !defined(SQXX_COLUMN_HPP_INCLUDED)
#error "Don't include column.impl.hpp directly, include column.hpp instead"
#endif

namespace sqxx {

template<typename T>
if_sqxx_db_type<T, T> column::val() const {
	return stmt.val<T>(idx);
}

} // namespace sqxx

#endif // SQXX_COLUMN_IMPL_HPP_INCLUDED

