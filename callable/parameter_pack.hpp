
// (c) 2014 Stephan Hohe

#if !defined(STHCXX_PACK_HPP_DEFINED)
#define STHCXX_PACK_HPP_DEFINED

namespace detail {

/** Count the number of types given to the template */
template<typename... Types>
struct pack_count;

template<>
struct pack_count<> {
	static const size_t value = 0;
};

template<typename Type, typename... Types>
struct pack_count<Type, Types...> {
	static const size_t value = pack_count<Types...>::value + 1;
};

/** Get the nth type given to the template */
template<size_t n, typename... Types>
struct pack_n;

template<size_t N, typename Type, typename... Types>
struct pack_n<N, Type, Types...> : pack_n<N-1, Types...> {
};

template<typename Type, typename... Types>
struct pack_n<0, Type, Types...> {
	typedef Type type;
};

} // namespace detail


template<typename... Types>
struct parameter_pack_traits {
	static const size_t count = detail::pack_count<Types...>::value;

	template<size_t N>
	using type = typename detail::pack_n<N>::type;
};

#endif // STHCXX_PACK_HPP_DEFINED

