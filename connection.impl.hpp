
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_CONNECTION_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_IMPL_HPP_INCLUDED

#if !defined(SQXX_CONNECTION_HPP_INCLUDED)
#error "Don't include connection.impl.hpp directly, include connection.hpp instead"
#endif

namespace sqxx {

//template<typename Callable>
//void connection::create_function(const std::string &name, Callable fun) {
//	create_function<Callable>(name.c_str(), std::forward<Callable>(fun));
//}
//
//template<typename Function, Function *Fun>
//std::enable_if_t<std::is_function<Function>::value, void>
//connection::create_function(const std::string &name) {
//	create_function<Function, Fun>(name.c_str());
//}

template<typename Callable>
void connection::create_collation(const char *name, Callable coll) {
	collation_function_t stdfun(coll);
	create_collation(name, stdfun);
}
template<typename Callable>
void connection::create_collation(const std::string &name, Callable coll) {
	create_collation(name.c_str(), std::forward<Callable>(coll));
}

template<typename Callable>
void connection::create_collation_stdstr(const char *name, Callable coll) {
	collation_function_stdstr_t stdfun(coll);
	create_collation_stdstr(name, stdfun);
}

} // namespace sqxx

#endif // SQXX_CONNECTION_IMPL_HPP_INCLUDED

