
// (c) 2013 Stephan Hohe

#if !defined(SQXX_BACKUP_HPP_INCLUDED)
#define SQXX_BACKUP_HPP_INCLUDED

#include "sqxx.hpp"

struct sqlite3_backup;

namespace sqxx {

template <typename Stepped>
class stepping_iterator {
private:
	Stepped *s;

public:
	explicit stepping_iterator(Stepped *s_arg = nullptr) : s(s_arg) {
	}

	Stepped& operator*() const {
		return *s;
	}
	stepping_iterator& operator++() {
		if (!s->step())
			s = nullptr;
	}

	bool operator==(const stepping_iterator &other) const {
		return (s == other.s);
	}
	bool operator!=(const stepping_iterator &other) const {
		return !(*this == other);
	}
};


class backup {
private:
	sqlite3_backup *handle;

public:
	backup(connection &dest, const char *ddb, connection &source, const char *sdb);
	~backup();

	bool step(int pages=1);
	void run() {
		step(-1);
	}
	int remaining();
	int pagecount();

	// Support for iterator access to walk over the backup steps.

	typedef stepping_iterator<backup> iterator;
	iterator begin() {
		return iterator(this);
	}
	iterator end() {
		return iterator();
	}

	sqlite3_backup* raw() {
		return handle;
	}
};

} // namesapce sqxx

#endif // SQXX_BACKUP_HPP_INCLUDED

