
/* Explore behavior of overloaded template functions that are selectively
 * enabled by enable_if<>
 */

#include <string>
#include <utility>

namespace sqxx {
	struct blob {};
}

// Check if type T is any of the other ones listed in Ts...
template<typename T, typename... Ts>
struct is_any {
	static const bool value = false;
};

template<typename T, typename T2, typename... Ts>
struct is_any<T, T2, Ts...> {
	static const bool value = std::is_same<T, T2>::value || is_any<T, Ts...>::value;
};


// std::enable_if<> that enables the overload if T is one of Ts...
template<typename R, typename T, typename... Ts>
using enable_if_any = typename std::enable_if<is_any<T, Ts...>::value, R>::type;


/*
template<typename T>
struct is_refparam : is_any<T, std::string, sqxx::blob> {};

template<typename T>
struct is_valparam : is_any<T, int, int64_t, double> {};

template<>
struct is_refparam<std::string> : std::true_type {};

template<typename T>
struct is_valparam : std::false_type {};
template<>
struct is_valparam<int> : std::true_type {};
template<>
struct is_valparam<int64_t> : std::true_type {};
template<>
struct is_valparam<double> : std::true_type {};
*/

// Types that don't have the option to be copied in bind()
/*
template<T>
struct is_bind_nocopy : std::false_type {};
template<>
struct is_bind_nocopy<int> : std::true_type {};
template<>
struct is_bind_nocopy<int64_t> : std::true_type {};
template<>
struct is_bind_nocopy<double> : std::true_type {};

// Types that can be copied by bind() and are passed by value
template<typename T>
using is_bind_copy_val = std::is_same<T, const char*>
*/

struct param {
	/*
	void bind(int) {};
	void bind(int64_t) {};
	void bind(double) {};
	void bind(const char *, bool=false) {};
	void bind(const std::string&, bool=false) {};
	*/

	template<typename T>
	enable_if_any<void, T, int, int64_t, double> bind(T v) {};

	template<typename T>
	enable_if_any<void, T, const char*> bind(T v, bool copy=true) {};

	template<typename T>
	enable_if_any<void, T, std::string, sqxx::blob> bind(const T &v, bool copy=true) {};
};

int main() {
	param p;
	std::string s;

	p.bind("abc");
	p.bind(s, false);
	//p.bind((unsigned)1);
	p.bind<std::string>("abc");
	p.bind<const char*>("abc", false);
}

