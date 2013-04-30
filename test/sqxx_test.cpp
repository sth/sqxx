
#include "sqlitepp.hpp"

#include <boost/test/unit_test.hpp>

namespace sqxx = sqlitepp;

BOOST_AUTO_TEST_SUITE(sqxx_cn)

struct db {
	sqxx::connection conn;
	db() : conn(":memory:") {
	}
};

struct tab : db {
	tab() : db() {
		conn.exec("create table tst (a integer, b integer)");
		conn.exec("insert into tst (a, b) values (1, 11), (2, 22)");
		conn.exec("create table types (i, l, d, s, b, n)");
		conn.exec("insert into types (i, l, d, s, b, n) values (1, 1000000000000, 1.5, 'abc', 'bin', NULL)");
	}
};

BOOST_AUTO_TEST_CASE(database) {
	db ctx;
	//BOOST_CHECK_EQUAL(ctx.conn.filename(), ":memory:");
	BOOST_CHECK_EQUAL(ctx.conn.readonly(), false);
	BOOST_CHECK_EQUAL(ctx.conn.total_changes(), 0);
	// test except
	ctx.conn.status(0);
}

BOOST_AUTO_TEST_CASE(table) {
	tab ctx;
}

BOOST_AUTO_TEST_CASE(result) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from tst where a = 1");
	sqxx::result r = st.run();
	BOOST_CHECK(r);
	BOOST_CHECK_EQUAL(r.col_count(), 1);
}

BOOST_AUTO_TEST_CASE(column_val) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from types where a = 1");
	sqxx::result r = st.run();
	BOOST_CHECK_EQUAL(r.col(0).val<int>(), 1);
	BOOST_CHECK_EQUAL(r.col(1).val<int64_t>(), 1000000000000L);
	BOOST_CHECK_EQUAL(r.col(2).val<double>(), 2.5);
	BOOST_CHECK_EQUAL(r.col(3).val<const char*>(), "abc");
	BOOST_CHECK_EQUAL(r.col(3).val<std::string>(), "abc");
	// TODO:
	//BOOST_CHECK(r.col(5).isnull());
}

BOOST_AUTO_TEST_CASE(column_cast) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from tst where a = 1");
	sqxx::result r = st.run();
	BOOST_CHECK_EQUAL(static_cast<int>(r.col(0)), 11);
	BOOST_CHECK_EQUAL(static_cast<int64_t>(r.col(0)), 11);
	BOOST_CHECK_EQUAL(static_cast<double>(r.col(0)), 11.0);
	BOOST_CHECK_EQUAL(static_cast<const char*>(r.col(0)), "11");
	BOOST_CHECK_EQUAL(static_cast<std::string>(r.col(0)), "11");
}

BOOST_AUTO_TEST_CASE(column_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select * types where a = 1");
	sqxx::result r = st.run();
	int v1 = r.col(0);
	BOOST_CHECK_EQUAL(v1, 11);
	int64_t v2 = r.col(0);
	BOOST_CHECK_EQUAL(v2, 11);
	//BOOST_CHECK_EQUAL(static_cast<double>(r.col(0)), 11.0);
	//BOOST_CHECK_EQUAL(static_cast<const char*>(r.col(0)), "11");
	//BOOST_CHECK_EQUAL(static_cast<std::string>(r.col(0)), "11");
}

/*
BOOST_AUTO_TEST_CASE(statement_result_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from tst where a = 1");
	sqxx::result r = st.run();
	BOOST_CHECK_EQUAL(r.col(0), 11);
	BOOST_CHECK_EQUAL(r.col(0), "11");
	BOOST_CHECK_EQUAL(r.col(0).val<std::string>(), "11");
}
*/

BOOST_AUTO_TEST_CASE(statement_result_iterator) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from tst order by a");
	int i = 0;
	for (auto r : st.run()) {
		++i;
		BOOST_CHECK(r);
		BOOST_CHECK_EQUAL(r.col_count(), 1);
		BOOST_CHECK_EQUAL(r.col(0).val<int>(), 11*i);
	}
}

BOOST_AUTO_TEST_CASE(commit_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_commit_handler([&]() { called = true; return 0; });
	ctx.conn.exec("begin transaction");
	ctx.conn.exec("select b from tst where a = 1");
	ctx.conn.exec("commit transaction");
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_SUITE_END()

