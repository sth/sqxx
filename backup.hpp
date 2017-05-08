
// (c) 2013 Stephan Hohe

#if !defined(SQXX_BACKUP_HPP_INCLUDED)
#define SQXX_BACKUP_HPP_INCLUDED

#include "connection.hpp"

struct sqlite3_backup;

namespace sqxx {

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
	class step_iterator {
	private:
		backup *b;
		int stepsize;

	public:
		explicit step_iterator(backup *b_arg = nullptr, int stepsize_arg = 1)
			: b(b_arg), stepsize(stepsize_arg) {
		}
		step_iterator(const step_iterator&) = delete;
		step_iterator& operator=(const step_iterator&) = delete;
		step_iterator(step_iterator&&) = default;
		step_iterator& operator=(step_iterator&&) = default;

		backup& operator*() const {
			// TODO: return curpagenum or sth.?
			return *b;
		}

		step_iterator& operator++() {
			if (!b->step(stepsize))
				b = nullptr;
			return *this;
		}
		// Not reasonably implementable with correct return type:
		void operator++(int) {
			++*this;
		}

		bool operator==(const step_iterator &other) const {
			return (b == other.b);
		}
		bool operator!=(const step_iterator &other) const {
			return !(*this == other);
		}
	};

	/** Iterator interface, uses `step()` internally to iterate through the complete backup */
	step_iterator begin(int stepsize = 1) {
		return step_iterator(this, stepsize);
	}
	step_iterator end() {
		return step_iterator();
	}

	/** Access to raw `struct sqlite3_backup*` */
	sqlite3_backup* raw() {
		return handle;
	}
};

} // namesapce sqxx

#endif // SQXX_BACKUP_HPP_INCLUDED

