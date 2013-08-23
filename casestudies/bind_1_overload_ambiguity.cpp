
/** Ambiguity of simple overloads */

#include <stdint.h>

struct parameter {
	void bind(int);
	void bind(int64_t);
	void bind(double);
	void bind(const char*);
};

int main() {
	st s;
	unsigned l;
	s.bind(l);
}

