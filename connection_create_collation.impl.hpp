
#if !defined(SQXX_CONNECTION_CREATE_COLLATION_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_CREATE_COLLATION_IMPL_HPP_INCLUDED

// Function types with C calling convention for callbacks
// https://stackoverflow.com/a/5590050/
extern "C" typedef int sqxx_collation_compare_type(void*, int, const void*, int, const void*);

namespace sqxx {
namespace detail {

// Create a collation function from a callable object
template<typename Callable>
sqxx_collation_compare_type collation_compare_ptr;

template<typename Callable>
int collation_compare_ptr(void *data, int llen, const void *lstr, int rlen, const void *rstr) {
	Callable *callable = reinterpret_cast<Callable*>(data);
	try {
		return (*callable)(
				llen, reinterpret_cast<const char*>(lstr),
				rlen, reinterpret_cast<const char*>(rstr)
			);
	}
	catch (...) {
		handle_callback_exception("collation function");
		return 0;
	}
}


template<typename Function, Function *Fun>
sqxx_collation_compare_type collation_compare_staticfptr;

template<typename Function, Function *Fun>
int collation_compare_staticfptr(void *data, int llen, const void *lstr, int rlen, const void *rstr) {
	try {
		return Fun(
				llen, reinterpret_cast<const char*>(lstr),
				rlen, reinterpret_cast<const char*>(rstr)
			);
	}
	catch (...) {
		handle_callback_exception("collation function");
		return 0;
	}
}

void create_collation_register(sqlite3 *handle, const char *name, void *data,
		sqxx_collation_compare_type *fun, sqxx_appdata_destroy_type *destroy);

template<typename Function>
std::enable_if_t<detail::decays_to_function_v<Function>, void>
create_collation_dispatch(sqlite3 *handle, const char *name, Function &&fun) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;

	// We store the function pointer as sqlite user data.
	// There is no special cleanup necessary for this user data.
	FunctionType *fptr = fun;
	detail::create_collation_register(handle, name, reinterpret_cast<void*>(fptr),
			detail::collation_compare_ptr<FunctionType>, nullptr);
}

template<typename Callable>
std::enable_if_t<!detail::decays_to_function_v<Callable>, void>
create_collation_dispatch(sqlite3 *handle, const char *name, Callable &&callable) {
	typedef std::decay_t<Callable> CallableType;
	// We store a copy of the callable as sqlite user data.
	// We register a "destructor" that deletes this copy when the function
	// gets removed.
	CallableType *cptr = new CallableType(std::forward<Callable>(callable));
	detail::create_collation_register(handle, name, reinterpret_cast<void*>(cptr),
			detail::collation_compare_ptr<CallableType>, detail::appdata_destroy_object<CallableType>);
}

} // namespace detail


template<typename Callable>
void connection::create_collation(const char *name, Callable &&callable) {
	detail::create_collation_dispatch<Callable>(handle, name, std::forward<Callable>(callable));
}

template<typename Callable>
void connection::create_collation(const std::string &name, Callable &&callable) {
	create_collation<Callable>(name.c_str(), std::forward<Callable>(callable));
}

// A function specified as a template parameter
template<typename Function, Function *Fun>
void connection::create_collation(const char *name) {
	// Function type is known statically and we don't need to store any
	// special sqlite user data.
	detail::create_collation_register(handle, name, nullptr,
			detail::collation_compare_staticfptr<Function, Fun>, nullptr);
}

template<typename Function, Function *Fun>
void connection::create_collation(const std::string &name) {
	create_collation<Function, Fun>(name.c_str());
}

} // namespace sqxx

#endif // SQXX_CONNECTION_CREATE_COLLATION_IMPL_HPP_INCLUDED

