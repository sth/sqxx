# libsqxx

Lightweight, object oriented, fully featured C++ 11 wrapper around libsqlite3.

## Notable features

- Only minimal overhead over calling the C API functions directly
- Contains easy to use functions that get common tasks done quickly, even without experience
  with sqlite3
- Contains methods corresponding directly to the C API functions, so if you are familiar
  with the C API, that knowledge should translate directly.
- Contains support for advanced features like registering callbacks, collation functions
  of custom SQL functions, fully in C++. For example a lambda functions can be registered
  as a custom SQL function and then be called in SQL queries.

## Getting started

### Main classes

The two main classes are `sqxx::connection`, which represents a database
conection, and `sqxx::statement`, which represents a sql statement including
binding of parameters, executing the statement, and accessing the results of
the execution.

### Common usage

Untested at the moment and subject to change if something turns out to be a bad
idea.

    // opening a database
    sqxx::connection conn("example.db");

    // simple statement execution
    conn.exec("create table items (id integer, v integer)");
    conn.exec("insert into items (id, value) values (1, 11), (2, 22), (3, 33)");

    // getting results
    sqxx::statement s1 = conn.exec("select value from items where id = 1");
    std::cout << "id=1 value=" << s1.col(0).val<int>() << std::endl;

    // iterating over multiple result rows
    sqxx::statement s2 = conn.exec("select * from items where id = 1");
    for (auto &r : s2) {
       std::cout << r.col(1).val<int>() << std::endl;
    }

    // Prepared statements
    sqxx::statement s3 = conn.prepare("select value from items where id = ?");
    s3.param(0).bind_int(2);
    s3.run();
    std::cout << "id=2 value=" << s3.col(0).val<int>() << std::endl;

    //s3.reset();
	 //s3.clear_bindings();
    //s3.param(0) = 3;
    //s3.run();
    //int v = s3.col(0);
    //std::cout << "id=3 value=" << v << std::endl;


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
uses UTF-8 and assues the corresponding data is encoded UTF-8.

### Consistent naming for handler/callback functions

The sqlite3 API provides several functions that register different kinds
of callback functions. Despite all of them registering a callback function
for some event, they have quite different names in the C API, like
`sqlite3_commit_hook()`, `sqlite3_trace()`, `sqlite3_set_authorizer()` or
`sqlite3_busy_handler()`. `sqxx` doesn't directly follow these names, but
uses the more consistent naming scheme `set_..._handler()` for all of them.

