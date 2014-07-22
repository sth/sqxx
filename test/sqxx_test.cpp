
// (c) 2013 Stephan Hohe

#include "sqxx.hpp"
#include "column.hpp"
#include "function.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(sqxx_cn)

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
	BOOST_CHECK(!st.done());
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

BOOST_AUTO_TEST_CASE(column_typed_val) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select * from types where id = 1");
	st.run();
	BOOST_CHECK_EQUAL(st.col(1).val<int>(), 2);
	BOOST_CHECK_EQUAL(st.col(2).val<int64_t>(), 3000000000000L);
	BOOST_CHECK_EQUAL(st.col(3).val<double>(), 4.5);
	BOOST_CHECK_EQUAL(st.col(4).val<const char *>(), "abc");
	BOOST_CHECK_EQUAL(st.col(4).val<std::string>(), "abc");
}

/*
BOOST_AUTO_TEST_CASE(column_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select * from types where id = 1");
	st.run();
	int v1 = st.col<int>(1);
	BOOST_CHECK_EQUAL(v1, 2);
	int64_t v2 = st.col<int64_t>(1);
	BOOST_CHECK_EQUAL(v2, 2);
	double v3 = st.col<double>(1);
	BOOST_CHECK_EQUAL(v3, 2.0);
	const char *v4 = st.col<const char*>(1);
	BOOST_CHECK_EQUAL(v4, "2");
	std::string v5 = st.col<std::string>(1);
	BOOST_CHECK_EQUAL(v5, "2");
}
*/

BOOST_AUTO_TEST_CASE(statement_result_conversion) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items where id = 1");
	st.run();
	BOOST_CHECK_EQUAL(st.val<int>(0), 11);
	BOOST_CHECK_EQUAL(st.val<int64_t>(0), 11);
	BOOST_CHECK_EQUAL(st.val<double>(0), 11.0);
	BOOST_CHECK_EQUAL(st.val<const char*>(0), "11");
	BOOST_CHECK_EQUAL(st.val<std::string>(0), "11");
}

BOOST_AUTO_TEST_CASE(statement_result_next) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items order by id");
	st.run();
	BOOST_CHECK_EQUAL(st.val<int>(0), 11);
	st.next_row();
	BOOST_CHECK_EQUAL(st.val<int>(0), 22);
}

BOOST_AUTO_TEST_CASE(statement_result_iterator) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select v from items order by id");
	st.run();
	BOOST_CHECK_EQUAL(st.col_count(), 1);
	size_t rowcount = 0;
	for (auto i : st) {
		BOOST_CHECK_EQUAL(st.col(0).val<int>(), 11*(i+1));
		rowcount = i+1;
	}
	BOOST_CHECK_EQUAL(rowcount, 3);
}

BOOST_AUTO_TEST_CASE(statement_param_bind) {
	tab ctx;
	sqxx::statement st = ctx.conn.prepare("select id from types where s = ?");
	st.bind(0, "abc");
	st.run();
	BOOST_CHECK(!st.done());
	BOOST_CHECK_EQUAL(st.val<int>(0), 1);

	st.reset();
	st.clear_bindings();
	st.bind(0, "xyz");
	st.run();
	BOOST_CHECK(st.done());
}

BOOST_AUTO_TEST_CASE(commit_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_commit_handler([&]() { called = true; return 0; });
	ctx.conn.run("begin transaction");
	ctx.conn.run("update items set v = 111 where id = 1");
	ctx.conn.run("commit transaction");
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(rollback_handler) {
	tab ctx;
	bool called = false;
	ctx.conn.set_rollback_handler([&]() { called = true; });
	ctx.conn.run("begin transaction");
	ctx.conn.run("update items set v = 111 where id = 1");
	ctx.conn.run("rollback transaction");
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
	ctx.conn.run("update items set v = 111 where id = 1");
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
	ctx.conn.run(sql);
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
	ctx.conn.run(sql);
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
	BOOST_CHECK(!st.done());
	BOOST_CHECK_EQUAL(st.val<int>(0), 1);
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

	ctx.conn.run("create table tst (id integer, v varchar)");
	ctx.conn.run("insert into tst (id, v) values (1, 'abcde'), (2, 'abxyz')");
	auto st = ctx.conn.run("select id from tst where v = 'abc' collate prefix");
	BOOST_CHECK(called);

	std::vector<int> data;
	for (auto i : st) {
		i = i; // suppress "unused variable" warning
		data.push_back(st.val<int>(0));
	}

	BOOST_CHECK_EQUAL(data.size(), 1);
	BOOST_CHECK_EQUAL(data[0], 1);
}

BOOST_AUTO_TEST_CASE(create_function) {
	tab ctx;
	bool called = false;
	ctx.conn.create_function("plusfive", std::function<int (int)>([&](int i) {
		called = true;
		BOOST_CHECK_EQUAL(i, 11);
		return i+5;
	}));
	auto st1 = ctx.conn.run("select plusfive(v) from items where id = 1");
	BOOST_CHECK_EQUAL(st1.val<int>(0), 16);
	BOOST_CHECK(called);

	called = false;
	ctx.conn.create_function("plussix", [&](int i) {
		called = true;
		BOOST_CHECK_EQUAL(i, 11);
		return i+6;
	});
	auto st2 = ctx.conn.run("select plussix(v) from items where id = 1");
	BOOST_CHECK_EQUAL(st2.val<int>(0), 17);
	BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(create_aggregate) {
	tab ctx;
	int called;

	// Single parameter aggregate
	called = 0;
	ctx.conn.create_aggregate("sqsum", 0, [&called](int &sum, int v) {
		sum += v;
		called++;
	}, [](const int &sum) -> int {
		return sum * sum;
	});
	auto st1 = ctx.conn.run("select sqsum(v) from items where id <= 2");
	BOOST_CHECK_EQUAL(called, 2);
	BOOST_CHECK_EQUAL(st1.val<int>(0), (11+22)*(11+22));

	// Single parameter aggregate, with starting value != 0
	ctx.conn.create_aggregate("sqsum10", 10, [](int &sum, int v) {
		sum += v;
	}, [](const int &sum) -> int {
		return sum * sum;
	});
	auto st2 = ctx.conn.run("select sqsum10(v) from items where id <= 2");
	BOOST_CHECK_EQUAL(st2.val<int>(0), (10+11+22)*(10+11+22));

	// Multi parameter aggregate
	ctx.conn.create_aggregate("aggr", 0, [](int &sum, int v, int w) {
		sum += v*w;
	}, [](const int &sum) -> int {
		return sum + 5;
	});
	auto st3 = ctx.conn.run("select aggr(v, v+1) from items where id <= 2");
	BOOST_CHECK_EQUAL(st3.val<int>(0), 11*12 + 22*23 + 5);

	// Simple "reduce style" aggregate
	called = 0;
	ctx.conn.create_aggregate_reduce("rsum", 0, [&](int acc, int i) -> int {
		called++;
		return acc + i;
	});
	auto st4 = ctx.conn.run("select rsum(v) from items where id <= 2");
	BOOST_CHECK_EQUAL(called, 2);
	BOOST_CHECK_EQUAL(st4.val<int>(0), 33);
}

BOOST_AUTO_TEST_SUITE_END()

