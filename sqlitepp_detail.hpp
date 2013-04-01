
// (c) 2013 Stephan Hohe

#if !defined(STHCXX_SQLITEPP_DETAIL_HPP_INCLUDED)
#define STHCXX_SQLITEPP_DETAIL_HPP_INCLUDED

#include "sqlitepp.hpp"

namespace sqlitepp {

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

} // namespace sqlitepp

#endif // STHCXX_SQLITEPP_DETAIL_HPP_INCLUDED

