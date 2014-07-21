
// (c) 2013 Stephan Hohe

#if !defined(SQXX_ERROR_HPP_INCLUDED)
#define SQXX_ERROR_HPP_INCLUDED

#include <stdexcept>
#include <functional>

namespace sqxx {

// TODO: Check what of this is really used

/** An error thrown if some sqlite API function returns an error */
class error : public std::runtime_error {
public:
	int code;
	error(int code_arg, const char *what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
	error(int code_arg, const std::string &what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
};

class static_error : public error {
public:
	static_error(int code_arg);
};

class managed_error : public error {
private:
	char *sqlitestr;
public:
	managed_error(int code_arg, char *what_arg);
	~managed_error() noexcept;
};

// Handle exceptions thrown from C++ callbacks
typedef std::function<void (const char*, std::exception_ptr)> callback_exception_handler_t;
void set_callback_exception_handler(const callback_exception_handler_t &handler);

// Write exeptions to std::cerr
void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept;

//namespace detail {
	void handle_callback_exception(const char *cbname);
//}

} // namespace sqxx

#endif // SQXX_ERROR_HPP_INCLUDED

