
#include "setup.hpp"

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(sqxx_examples)

BOOST_AUTO_TEST_CASE(example_aggregate_geom) {
	struct geom_state { int64_t sum = 0; int64_t count = 0; };

	auto geom_step = [](geom_state &state, int64_t n) {
		state.count++;
		state.sum += n*n;
	};

	auto geom_final = [](const geom_state &state) -> double {
		return sqrt(state.sum * 1.0 / state.count);
	};

	tab ctx;
	ctx.conn.create_aggregate("geom", geom_state(), geom_step, geom_final);
	ctx.conn.run("SELECT geom(v) FROM items");
}

BOOST_AUTO_TEST_SUITE_END()

