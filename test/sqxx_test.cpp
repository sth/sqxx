
#include "sqxx.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(sqxx_cn)

struct db {
	sqxx::connection conn;
	db() : conn(":memory:") {
	}
};

struct tab : db {
	tab() : db() {
		conn.exec("create table items (id integer, v integer)");
		conn.exec("insert into items (id, v) values (1, 11), (2, 22), (3, 33)");
		conn.exec("create table types (id, i, l, d, s, b, n)");
		conn.exec("insert into types (id, i, l, d, s, b, n) values (1, 2, 3000000000000, 4.5, 'abc', 'bin', NULL)");
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
	sqxx::statement st = ctx.conn.prepare("select v from items where id = 1");
	st.run();
	BOOST_CHECK(!st.eof());
	BOOST_CHECK_EQUAL(st.col_count(), 1);
}

BOOST_AUTO_TEST_CASE(column_val) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select * from types where id = 1");
	st.run();
	BOOST_CHECK_EQUAL(st.col(1).val<int>(), 2);
	BOOST_CHECK_EQUAL(st.col(2).val<int64_t>(), 3000000000000L);
	BOOST_CHECK_EQUAL(st.col(3).val<double>(), 4.5);
	BOOST_CHECK_EQUAL(st.col(4).val<const char*>(), "abc");
	BOOST_CHECK_EQUAL(st.col(4).val<std::string>(), "abc");
	// TODO:
	//BOOST_CHECK(r.col(6).isnull());
}

BOOST_AUTO_TEST_CASE(column_cast) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items where id = 1");
	st.run();
	BOOST_CHECK_EQUAL(static_cast<int>(st.col(0)), 11);
	BOOST_CHECK_EQUAL(static_cast<int64_t>(st.col(0)), 11);
	BOOST_CHECK_EQUAL(static_cast<double>(st.col(0)), 11.0);
	BOOST_CHECK_EQUAL(static_cast<const char*>(st.col(0)), "11");
	BOOST_CHECK_EQUAL(static_cast<std::string>(st.col(0)), "11");
}

BOOST_AUTO_TEST_CASE(column_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select * from types where id = 1");
	st.run();
	int v1 = st.col(1);
	BOOST_CHECK_EQUAL(v1, 2);
	int64_t v2 = st.col(1);
	BOOST_CHECK_EQUAL(v2, 2);
	double v3 = st.col(1);
	BOOST_CHECK_EQUAL(v3, 2.0);
	const char *v4 = st.col(1);
	BOOST_CHECK_EQUAL(v4, "2");
	std::string v5 = static_cast<std::string>(st.col(1));
	BOOST_CHECK_EQUAL(v5, "2");
}

/*
BOOST_AUTO_TEST_CASE(statement_result_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select b from items where a = 1");
	st.run();
	BOOST_CHECK_EQUAL(st.col(0), 11);
	BOOST_CHECK_EQUAL(st.col(0), "11");
	BOOST_CHECK_EQUAL(st.col(0).val<std::string>(), "11");
}
*/

BOOST_AUTO_TEST_CASE(statement_result_next) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items order by id");
	st.run();
	BOOST_CHECK_EQUAL(st.col(0).val<int>(), 11);
	st.next_row();
	BOOST_CHECK_EQUAL(st.col(0).val<int>(), 22);
}

BOOST_AUTO_TEST_CASE(statement_result_iterator) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items order by id");
	int i = 0;
	st.run();
	for (auto& r : st) {
		++i;
		BOOST_CHECK(r);
		BOOST_CHECK_EQUAL(r.col_count(), 1);
		BOOST_CHECK_EQUAL(r.col(0).val<int>(), 11*i);
	}
	BOOST_CHECK_EQUAL(i, 3);
}

BOOST_AUTO_TEST_CASE(commit_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_commit_handler([&]() { called = true; return 0; });
	ctx.conn.exec("begin transaction");
	ctx.conn.exec("update items set v = 111 where id = 1");
	ctx.conn.exec("commit transaction");
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(rollback_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_rollback_handler([&]() { called = true; });
	ctx.conn.exec("begin transaction");
	ctx.conn.exec("update items set v = 111 where id = 1");
	ctx.conn.exec("rollback transaction");
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(update_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_update_handler([&](int op, const char *dbname, const char *tabname, int64_t rowid) {
		called = true;
		//BOOST_CHECK_EQUAL(op, SQLITE_UPDATE);
		BOOST_CHECK_EQUAL(dbname, "main");
		BOOST_CHECK_EQUAL(tabname, "items");
		BOOST_CHECK_EQUAL(rowid, 1);
	});
	ctx.conn.exec("update items set v = 111 where id = 1");
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(trace_handler) {
	tab ctx;
	bool called = false;
	std::string sql = "update items set v = 111 where id = 1";
	ctx.conn.set_trace_handler([&](const char *q) {
		called = true;
		BOOST_CHECK_EQUAL(q, sql);
	});
	ctx.conn.exec(sql);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(profile_handler) {
	tab ctx;
	bool called = false;
	std::string sql = "update items set v = 111 where id = 1";
	ctx.conn.set_profile_handler([&](const char *q, uint64_t usec) {
		called = true;
		BOOST_CHECK_EQUAL(q, sql);
	});
	ctx.conn.exec(sql);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(authorize_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_authorize_handler([&](int action, const char *d1, const char *d2, const char *d3, const char *d4) -> int {
		called = true;
		// TODO
		//std::cout << "params: " << d1 << "/" << d2 << "/" << d3 << "/" << d4 << std::endl;
		/*
		if (action == SQLITE_READ && d1 == "main" && d2 == "item" && d3 == ) {
			return SQLITE_IGNORE;
		}
		else {
			return SQLITE_OK;
		}
		*/
		return 0;
	});

	auto st = ctx.conn.prepare("select id, v from items where id = 1");
	BOOST_CHECK(called);
	st.run();
	BOOST_CHECK(!st.eof());
	BOOST_CHECK_EQUAL(st.col(0).val<int>(), 1);
	//BOOST_CHECK_EQUAL(static_cast<const void*>(res.col(1).val<const char*>()), static_cast<const void*>(nullptr));
}

BOOST_AUTO_TEST_CASE(create_collation) {
	tab ctx;
	bool called = false;
	// test collation function that only compares the common prefix
	ctx.conn.create_collation("prefix", [&](int llen, const char *lstr, int rlen, const char *rstr) -> int {
			called = true;
			int minlen;
			if (llen < rlen)
				minlen = llen;
			else
				minlen = rlen;
			return memcmp(lstr, rstr, minlen);
		});

	ctx.conn.exec("create table tst (id integer, v varchar)");
	ctx.conn.exec("insert into tst (id, v) values (1, 'abcde'), (2, 'abxyz')");
	auto st = ctx.conn.exec("select id from tst where v = 'abc' collate prefix");
	BOOST_CHECK(called);

	std::vector<int> data;
	for (auto &r : st) {
		data.push_back(r.col(0).val<int>());
	}

	BOOST_CHECK_EQUAL(data.size(), 1);
	BOOST_CHECK_EQUAL(data[0], 1);
}

BOOST_AUTO_TEST_SUITE_END()

