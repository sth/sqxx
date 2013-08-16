# libsqxx

Lightweight, object oriented, fully featured C++ 11 wrapper around libsqlite3.

## Main classes

The two main classes are `sqxx::connection`, which represents a database
conection, and `sqxx::statement`, which represents a sql statement including
binding of parameters, executing the statement, and accessing the results of
the execution.

## Example

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
    s3.param(0).bind(2);
    s3.run();
    std::cout << "id=2 value=" << s3.col(0).val<int>() << std::endl;

    s3.reset();
    s3.param(0) = 3;
    s3.run();
    int v = s3.col(0);
    std::cout << "id=3 value=" << v << std::endl;

