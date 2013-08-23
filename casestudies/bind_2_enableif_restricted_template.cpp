
#include <string>
#include <utility>

template<typename T>
struct is_dbtype : std::false_type {};

template<>
struct is_dbtype<int> : std::true_type {};
template<>
struct is_dbtype<int64_t> : std::true_type {};
template<>
struct is_dbtype<double> : std::true_type {};
template<>
struct is_dbtype<const char*> : std::true_type {};

struct p {
	template<typename T>
	typename std::enable_if<is_dbtype<T>::value, void>::type
	bind(T v) {};
	void bind(const std::string &s) {};
};

template<>
void p::bind<int>(int v) {
}

int main() {
	p a;
	a.bind(1);
	a.bind((long)1);
	std::string s;
	a.bind(s);
}

