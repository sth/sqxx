
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_CONNECTION_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_IMPL_HPP_INCLUDED

#if !defined(SQXX_CONNECTION_HPP_INCLUDED)
#error "Don't include connection.impl.hpp directly, include connection.hpp instead"
#endif

namespace sqxx {

template<typename Callable>
void connection::create_collation_stdstr(const char *name, Callable coll) {
	collation_function_stdstr_t stdfun(coll);
	create_collation_stdstr(name, stdfun);
}

} // namespace sqxx

#endif // SQXX_CONNECTION_IMPL_HPP_INCLUDED

