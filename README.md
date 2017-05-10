# libsqxx

Lightweight, object oriented, fully featured C++ 11 wrapper around libsqlite3.

## Notable features

- Modern C++ interface (range-base `for` iteration over sql results, registering
  C++ lambdas as SQL functions, ...)
- As far as possible no overhead over calling the C API functions directly
- Support for exotic sqlite features (register C++ lamdas as sqlite hook functions,
  query runtime status information, creating wal checkpoints, ...)
- Easy access and interoperability with the underlying sqlite C-Api in case the
  provided C++ abstraction isn't enough for some special use case (though
  intention is to make this unnecessary for all but the most exotic requirements)
- Easy to use convenience functions that get common tasks done quickly, even
  without experience with sqlite3
- Methods corresponding directly to the C API functions, so if you are familiar
  with the C API, that knowledge should translate directly
- Support for advanced features like registering callbacks, collation functions
  or custom SQL functions, fully in C++. For example a lambda functions can be
  registered as a custom SQL function and then be called in SQL queries.

(TODO: Add section explaining detailed performace characteristics;
Add section explaining differences to C Api and missing parts (intentional or still to do))

### General implementation features

- Only minimal or no overhead over calling the C API functions directly
- No pollution of the global namespace with sqlite symbols.
- Register C++ functions/lambdas/... as SQL functions or SQL aggregates
- Register C++ functions/lambdas/... as sqlite3 callbacks/hooks
- Exception safe C/C++ boundary for callbacks/...
- Access to raw sqlite handles to use C API functions directly, if
  desired. (In case some functionality you want to use is missing in
  this C++ wrapper)


### Missing features

- No support to receive text values in UTF-16 encoding. All text functions use UTF-8. 

    Sqlite contains API functions to access values in different kinds of UTF-16 encoding.
    sqxx doesn't contain support for this, it accesses all values as UTF-8.
    Note that sqlite stores all data internally as ?, the different encoding
    variants for text functions just define how the values are converted before
    they are returned by the API.<sup>[1][?]</sup> With sqxx all values will
    always be returned as UTF-8, without the need to explicitly specify this.

- `sqlite3_vfs_*` virtual file system handlers

    This feature seems to be very specific and not usually used in normal operation.
	 (TODO: reference to sqlite docs)

- Some implemented features are not covered by test cases and maybe untested/incomplete:
  - varags SQL functions
  - blob values
  - `sqlite3_backup*` wrappers
- No custom destructors for text/blob values given to sqlite

## License

You can use the library in any programs you like, closed or open source,
commercial or free. The library is licened under the LGPL, so when you make
changes to *the library itself* and distribute the modified library, you have
to make the changed source of the library available. For details see the
LICENSE.md file.

## Getting started

### Main classes

The two main classes are `sqxx::connection`, which represents a database
connection, and `sqxx::statement`, which represents a SQL statement including
binding of parameters, executing the statement, and accessing the results of
the execution.

### Quick usage overview

<#include "example.cpp">

### Api

#### Creating a database connection

The constructor of `sqxx::connection` takes the database name as a parameter:

    sqxx::connection conn("/path/to/dbfile");

Alternatively, you can also use the default constructor to create the object
and then later open a database with the `open()` method:

    sqxx::connection conn;
    conn.open("/path/to/dbfile");

To create a temporary in-memory database, use the special string `":memory:"` as
a database file name. For more details, also see the documentation of the wrapped
C API function.

#### Executing queries

The most common use for such a connection object is to execute SQL queries with
`exec()`:

    conn.exec("create table items (id integer, v integer)");
    conn.exec("insert into items (id, value) values (1, 11), (2, 22), (3, 33)");

The exec() method returns a `sqxx::statement` that can be used to access the
results of a query:

	 sqxx::statement st = conn.exec("select * from items where id = 1");

# Accessing result values

Results are accessed one row after another. The values of in the current row
are usually accessed with the `val()` method. This is a template that takes
one type parameter, specifying to which type the database value should be
converted. It's parameter is the column index that should be accessed:

    // Get the value in column 0 as an int
    int i = st.val<int>(0);

    // Get the value in column 1 as a string
    std::string s = st.val<std::string>(1);

The type parameter must be one of the types supported by this sqxx, which are
`int`, `int64_t`, `double`, `const char*`, `std::string` and `sqxx:blob`.
These types map directly to the types supported by the underlying C API.

Since the values in a sqlite3 database are stored without type information,
every value can be accessed as any type. So `val<double>(0)`, will receive the
value as a double, even if it previously was stored as an int or string. The
underlying sqlite3 C API will convert the value if necessary.

#### Accessing multiple result rows

The result rows are accessed one after another,


The statements `done()` method can be used to check if there are 

## Changed semantics compared to the C API

### Parameter indexes in prepared statements start with zero (not one).
 
The sqlite3 C API uses parameter indexes starting with one, but this is
inconsistent with for example column indexes (which start with zero).
Additionally indexes starting with one are generally surprising and
uncommon ind C/C++.

The index zero is used as an error value in in the C API, but in C++
we can use exceptions instead.

sqxx therefore uses zero-based indexing for parameters.

### Everything is UTF-8

The C API usually provides several variants of its functions that can be used
to access data in UTF-8, UTF-16 little endian or UTF-16 big endian. This
complexity was omitted in this library and `sqxx` always uses the UTF-8
versions of functions and expects all strings to be encoded in UTF-8.
Whenever there is a choice between several encodings in the C API, `sqxx`
uses UTF-8 and assumes the corresponding data is encoded UTF-8.

### Consistent naming for handler/callback functions

The sqlite3 API provides several functions that register different kinds
of callback functions. Despite all of them registering a callback function
for some event, they have quite different names in the C API, like
`sqlite3_commit_hook()`, `sqlite3_trace()`, `sqlite3_set_authorizer()` or
`sqlite3_busy_handler()`. `sqxx` doesn't directly follow these names, but
uses the more consistent naming scheme `set_..._handler()` for all of them.

