
#if !defined(SQXX_TEST_SETUP_HPP_INCLUDED)
#define SQXX_TEST_SETUP_HPP_INCLUDED

#include "sqxx.hpp"

struct db {
	sqxx::connection conn;
	// Create an in-memory database
	db() : conn(":memory:") {
	}
};

struct tab : db {
	// Create an in-memory database and set up some test tables
	tab() : db() {
		conn.run("create table items (id integer, v integer)");
		conn.run("insert into items (id, v) values (1, 11), (2, 22), (3, 33)");
		conn.run("create table types (id, i, l, d, s, b, n)");
		conn.run("insert into types (id, i, l, d, s, b, n) values (1, 2, 3000000000000, 4.5, 'abc', 'bin', NULL)");
	}
};


#endif // SQXX_TEST_SETUP_HPP_INCLUDED

