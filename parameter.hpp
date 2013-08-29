
// (c) 2013 Stephan Hohe

#if !defined(SQXX_PARAMETER_HPP_INLCUDED)
#define SQXX_PARAMETER_HPP_INLCUDED

#include "datatypes.hpp"
#include "sqxx.hpp"
#include <string>

namespace sqxx {

class statement;

/**
 * Represents a parameter of a prepared statement
 */
class parameter {
public:
	statement &stmt;
	const int idx;

	parameter(statement &a_stmt, int a_idx);

	/** sqlite3_parameter_name() */
	const char* name() const;

	/** Bind a value to this parameter
	 *
	 * For details see statement::bind()
	 */
	void bind();

	template<typename T>
	if_selected_type<T, void, int, int64_t, double>
	bind(T value);

	template<typename T>
	if_selected_type<T, void, const char*>
	bind(T value, bool copy=true);

	template<typename T>
	if_selected_type<T, void, std::string, blob>
	bind(const T &value, bool copy=true);
};

inline void parameter::bind() { stmt.bind(idx); }

template<typename T>
if_selected_type<T, void, int, int64_t, double>
parameter::bind(T value) { stmt.bind<T>(idx, value); }

template<typename T>
if_selected_type<T, void, const char*>
parameter::bind(T value, bool copy) { stmt.bind<T>(idx, value, copy); }

template<typename T>
if_selected_type<T, void, std::string, blob>
parameter::bind(const T &value, bool copy) { stmt.bind<T>(idx, value, copy); }

} // namespace sqxx

#endif // SQXX_PARAMETER_HPP_INLCUDED

