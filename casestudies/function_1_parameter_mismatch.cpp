
#include "sqxx.hpp"
#include "func.hpp"

long f1() {
}

int main() {
	sqxx::connection conn(":memory:");

	conn.create_function("f1", std::function<long ()>(f1));
}

