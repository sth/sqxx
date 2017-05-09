
// Implementation of connection::create_function

#if !defined(SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED)
#define SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED

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
		sqxx_function_call_type *fun, sqxx_function_destroy_type *destroy);

} // namespace detail



// A function pointer passed as a function argument
template<typename Function>
std::enable_if_t<detail::decays_to_function_v<Function>, void>
connection::create_function(const char *name, Function fun) {
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
connection::create_function(const char *name, Callable callable) {
	typedef std::decay_t<Callable> CallableType;
	typedef callable_traits<CallableType> traits;

	// We store a copy of the callable as sqlite user data.
	// We register a "destructor" that deletes this copy when the function
	// gets removed.
	CallableType *cptr = new CallableType(callable);
	detail::create_function_register(handle, name, traits::argc,
			reinterpret_cast<void*>(cptr), detail::function_call_ptr<CallableType>, detail::function_destroy_object<CallableType>);
}


// A function specified as a template parameter
template<typename Function, Function *Fun>
std::enable_if_t<std::is_function<Function>::value, void>
connection::create_function(const char *name) {
	typedef std::remove_pointer_t<std::decay_t<Function>> FunctionType;
	typedef callable_traits<FunctionType> traits;

	// Function type is known statically and we don't need to store any
	// special sqlite user data.
	detail::create_function_register(handle, name, traits::argc,
			nullptr, detail::function_call_staticfptr<Function, Fun>, nullptr);
}

} // namespace sqxx

#endif // SQXX_CONNECTION_CREATE_FUNCTION_IMPL_HPP_INCLUDED

