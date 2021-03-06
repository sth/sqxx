
// Implementation of connection::create_function

#if !defined(SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED

// Function types with C calling convention for callbacks
// https://stackoverflow.com/a/5590050/
extern "C" typedef void sqxx_function_call_type(sqlite3_context*, int, sqlite3_value**);

namespace sqxx {
namespace detail {

// Create a SQL function from a callable object
template<typename Callable>
sqxx_function_call_type function_call_ptr;

template<typename Callable>
void function_call_ptr(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	typedef callable_traits<Callable> traits;
	context ctx(handle);
	Callable *callable = reinterpret_cast<Callable*>(sqlite3_user_data(handle));
	try {
		if (argc != traits::argc) {
			ctx.result_error_misuse();
			return;
		}
		ctx.result<typename traits::return_type>(
				apply_value_array<traits::argc, typename traits::return_type>(*callable, argv)
			);
	}
	catch (const error &e) {
		ctx.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		ctx.result_error_nomem();
	}
	catch (const std::exception &e) {
		ctx.result_error(e.what());
	}
	catch (...) {
		ctx.result_error_misuse();
	}
}


// Create a SQL function from a statically known function pointer

template<typename Function, Function *Fun>
sqxx_function_call_type function_call_staticfptr;

template<typename Function, Function *Fun>
void function_call_staticfptr(sqlite3_context *handle, int argc, sqlite3_value** argv) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;
	context ctx(handle);
	try {
		if (argc != traits::argc) {
			ctx.result_error_misuse();
			return;
		}
		ctx.result<typename traits::return_type>(
				apply_value_array<traits::argc, typename traits::return_type>(Fun, argv)
			);
	}
	catch (const error &e) {
		ctx.result_error_code(e.code);
	}
	catch (const std::bad_alloc &) {
		ctx.result_error_nomem();
	}
	catch (const std::exception &e) {
		ctx.result_error(e.what());
	}
	catch (...) {
		ctx.result_error_misuse();
	}
}

void create_function_register(sqlite3 *handle, const char *name, int narg, void *data,
		sqxx_function_call_type *fun, sqxx_appdata_destroy_type *destroy);

template<typename Function>
std::enable_if_t<detail::decays_to_function_v<Function>, void>
create_function_dispatch(sqlite3 *handle, const char *name, Function fun) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;

	// We store the function pointer as sqlite user data.
	// There is no special cleanup necessary for this user data.
	FunctionType *fptr = fun;
	detail::create_function_register(handle, name, traits::argc,
			reinterpret_cast<void*>(fptr), detail::function_call_ptr<FunctionType>, nullptr);
}

template<typename Callable>
std::enable_if_t<!detail::decays_to_function_v<Callable>, void>
create_function_dispatch(sqlite3 *handle, const char *name, Callable &&callable) {
	typedef std::decay_t<Callable> CallableType;
	typedef callable_traits<CallableType> traits;

	// We store a copy of the callable as sqlite user data.
	// We register a "destructor" that deletes this copy when the function
	// gets removed.
	CallableType *cptr = new CallableType(std::forward<Callable>(callable));
	detail::create_function_register(handle, name, traits::argc,
			reinterpret_cast<void*>(cptr), detail::function_call_ptr<CallableType>, detail::appdata_destroy_object<CallableType>);
}

} // namespace detail


template<typename Callable>
void connection::create_function(const char *name, Callable &&callable) {
	detail::create_function_dispatch<Callable>(handle, name, std::forward<Callable>(callable));
}

// A function specified as a template parameter
template<typename Function, Function *Fun>
void connection::create_function(const char *name) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;

	// Function type is known statically and we don't need to store any
	// special sqlite user data.
	detail::create_function_register(handle, name, traits::argc,
			nullptr, detail::function_call_staticfptr<Function, Fun>, nullptr);
}

template<typename Function, Function *Fun>
void connection::create_function(const std::string &name) {
	create_function<Function, Fun>(name.c_str());
}

// We could add overloads that allow removing a function by not explicitly specifying
// it's arity. But is that really usefull?
//
// A call like `remove_function("foo", some_fun)` might be misleading, since it wouldn't
// just remove `some_fun` if it's registered, but any "foo" function with the same arity.
//
//template<typename Callable>
//void connection::remove_function(const char *name, Callable &&/*callable*/) {
//	remove_function(name, NArgs);
//}
//
//template<typename Function>
//void connection::remove_function(const char *name) {
//	constexpr int NArgs = callable_traits<std::decay_t<Callable>>::cargs;
//	remove_function<Function>(name);
//}

} // namespace sqxx

#endif // SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED

