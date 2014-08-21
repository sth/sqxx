
// (c) 2013, 2014 Stephan Hohe

#if !defined(SQXX_PARAMETER_IMPL_HPP_INCLUDED)
#define SQXX_PARAMETER_IMPL_HPP_INCLUDED

#if !defined(SQXX_PARAMETER_HPP_INCLUDED)
#error "Don't include parameter.impl.hpp directly, include parameter.hpp instead"
#endif

namespace sqxx {

inline void parameter::bind() {
	stmt.bind(idx);
}

template<typename T>
if_selected_type<T, void, int, int64_t, double>
parameter::bind(T value) {
	stmt.bind<T>(idx, value);
}

template<typename T>
if_selected_type<T, void, const char*>
parameter::bind(T value, bool copy) {
	stmt.bind<T>(idx, value, copy);
}

template<typename T>
if_selected_type<T, void, std::string, blob>
parameter::bind(const T &value, bool copy) {
	stmt.bind<T>(idx, value, copy);
}

} // namespace sqxx

#endif // SQXX_PARAMETER_IMPL_HPP_INCLUDED

