
// (c) 2013 Stephan Hohe

#if !defined(SQXX_SQXX_HPP_INCLUDED)
#define SQXX_SQXX_HPP_INCLUDED

#include "datatypes.hpp"
#include <stdexcept>
#include <vector>
#include <memory>
#include <utility>

#include "error.hpp"
#include "connection.hpp"
#include "statement.hpp"
#include "parameter.hpp"
#include "column.hpp"

namespace sqxx {

// Handle exceptions thrown from C++ callbacks
typedef std::function<void (const char*, std::exception_ptr)> callback_exception_handler_t;
void set_callback_exception_handler(const callback_exception_handler_t &handler);

// Write exeptions to std::cerr
void default_callback_exception_handler(const char *cbname, std::exception_ptr ex) noexcept;

} // namespace sqxx

#endif // SQXX_SQXX_HPP_INCLUDED

