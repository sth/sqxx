
// (c) 2013 Stephan Hohe

#if !defined(SQXX_DETAIL_HPP_INCLUDED)
#define SQXX_DETAIL_HPP_INCLUDED

#include "sqxx.hpp"

namespace sqxx {

// TODO: Check what of this is really used

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

class recent_error : public error {
public:
	recent_error(sqlite3 *handle);
};

} // namespace sqxx

#endif // SQXX_DETAIL_HPP_INCLUDED

