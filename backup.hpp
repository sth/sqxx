
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


/**
 * Copying of tables from one DB to another.
 *
 * Wraps the C API struct `sqlite3_backup` and associated functions.
 */
class backup {
private:
	sqlite3_backup *handle;

public:
	/** Wraps [`sqlite3_backup_init()`](http://www.sqlite.org/c3ref/backup_finish.html) */
	backup(connection &dest, const char *ddb, connection &source, const char *sdb);
	/** Wraps [`sqlite3_backup_finish()`](http://www.sqlite.org/c3ref/backup_finish.html) */
	~backup();

	/** Wraps [`sqlite3_backup_step()`](http://www.sqlite.org/c3ref/backup_finish.html) */
	bool step(int pages=1);
	void run() {
		step(-1);
	}
	/** Wraps [`sqlite3_backup_remaining()`](http://www.sqlite.org/c3ref/backup_finish.html) */
	int remaining();
	/** Wraps [`sqlite3_backup_pagecount()`](http://www.sqlite.org/c3ref/backup_finish.html) */
	int pagecount();

	// Support for iterator access to walk over the backup steps.

	typedef stepping_iterator<backup> iterator;
	/** Iterator interface, uses `step()` internally to iterate through the complete backup */
	iterator begin() {
		return iterator(this);
	}
	iterator end() {
		return iterator();
	}

	/** Access to raw `struct sqlite3_backup*` */
	sqlite3_backup* raw() {
		return handle;
	}
};

} // namesapce sqxx

#endif // SQXX_BACKUP_HPP_INCLUDED

