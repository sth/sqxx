
// (c) 2013 Stephan Hohe

#if !defined(SQXX_DATATYPES_HPP_INCLUDED)
#define SQXX_DATATYPES_HPP_INCLUDED

#include <utility>
#include <string>

namespace sqxx {

struct blob {
	const void *data;
	uint64_t length;
	blob(const void *data_arg, int length_arg) : data(data_arg), length(length_arg) {
	}
};

template<typename T=int64_t>
struct tcounter {
	T current;
	T highwater;
	tcounter() = default;

	template<typename U>
	tcounter(const tcounter<U> &other) : current(other.current), highwater(other.highwater) {
	}
};

typedef tcounter<int64_t> counter;

namespace detail {

// Check if type T is the same as any of the ones listed in Ts...
template<typename T, typename... Ts>
struct is_selected;

template<typename T>
struct is_selected<T> {
	static const bool value = false;
};

template<typename T, typename T2, typename... Ts>
struct is_selected<T, T2, Ts...> {
	static const bool value = std::is_same<T, T2>::value || is_selected<T, Ts...>::value;
};

template<typename T>
using decays_to_function = std::is_function<std::remove_pointer_t<std::decay_t<T>>>;

template<typename T>
constexpr bool decays_to_function_v = decays_to_function<T>::value;

}

/** `std::enable_if<>` for all types in the `Ts...` list */
template<typename T, typename R, typename... Ts>
using if_selected_type = typename std::enable_if<detail::is_selected<T, Ts...>::value, R>::type;

/** `std::enable_if<>` for the types supported by sqxx's db interface
 * (`int`, `int64_t`, `double`, `const char*`, `std::string`, `blob`)
 */
template<typename T, typename R>
using if_sqxx_db_type = if_selected_type<T, R,
		int, int64_t, double, const char*, std::string, blob>;

enum class datatype {
	INTEGER = 1,
	FLOAT = 2,
	TEXT = 3,
	BLOB = 4,
	NULLVALUE = 5,
};

// Helper template to denote intentionally unused parameters.
// Quiets compiler warnings about them.
template<typename T>
void unused(T) {
}

} // namespace sqxx

#endif // SQXX_DATATYPES_HPP_INCLUDED

