
#if !defined(SQXX_CONNECTION_CALLBACKS_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_CALLBACKS_IMPL_HPP_INCLUDED

#include "callable/callable.hpp"
#include "context.hpp"
#include "value.hpp"

// Function types with C calling convention for callbacks
// https://stackoverflow.com/a/5590050/
extern "C" typedef void sqxx_function_call_type(sqlite3_context*, int, sqlite3_value**);
extern "C" typedef void sqxx_function_destroy_type(void*);

extern "C" void* sqlite3_user_data(sqlite3_context*);

namespace sqxx {
namespace detail {

// Apply an array of sqlite3_value's to a function

template<int I, int N, typename R, typename Fun>
struct apply_value_array_st {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value **argv, Values&&... args) {
		return apply_value_array_st<I+1, N, R, Fun>::template apply<Values..., value>(
				std::forward<Fun>(f),
				argv,
				std::forward<Values>(args)..., 
				value(argv[I])
			);
	}
};

template<int I, typename R, typename Fun>
struct apply_value_array_st<I, I, R, Fun> {
	template<typename... Values>
	static R apply(Fun f, sqlite3_value** argv, Values&&... args) {
		unused(argv);
		return f(std::forward<Values>(args)...);
	}
};

template<int N, typename R, typename Fun, typename... BaseArgs>
R apply_value_array(Fun &&f, sqlite3_value **argv, BaseArgs&&... baseargs) {
	return apply_value_array_st<0, N, R, Fun>::apply(
			std::forward<Fun>(f),
			argv,
			std::forward<BaseArgs>(baseargs)...
		);
}


template<typename T>
sqxx_function_destroy_type function_destroy_object;

template<typename T>
void function_destroy_object(void *data) {
	delete reinterpret_cast<T*>(data);
}


// This is useful when an object is stored in memory that is managed by sqlite.
template<typename T>
struct destruct_only {
	void operator()(T *ptr) {
		if (ptr) {
			ptr->~T();
		}
	}
};

// A std::unique_ptr<> variant that takes ownership of the pointed-to object,
// but not the pointed-to memory. When the `ownedobj_ptr<>` goes out of scope,
// if calls the pointed-to object's destructor, but doesn't free the
// associated memory.
template<typename T>
using ownedobj_ptr = std::unique_ptr<T, destruct_only<T>>;

} // namespace detail
} // namespace sqxx

#endif // SQXX_CONNECTION_CALLBACKS_IMPL_HPP_INCLUDED

