
#include "sqlitepp_backup.hpp"
#include "sqlitepp_detail.hpp"
#include <sqlite3.h>

namespace sqlitepp {

backup::backup(connection &dest, const char *ddb, connection &source, const char *sdb)
	: handle(sqlite3_backup_init(source.raw(), sdb, dest.raw(), ddb)) {
	if (!handle)
		throw recent_error(dest.raw());
}

backup::~backup() {
	if (handle) {
		sqlite3_backup_finish(handle);
	}
}

bool backup::step(int pages) {
	int rv;
	rv = sqlite3_backup_step(handle, pages);
	if (rv == SQLITE_OK)
		return true;
	else if (rv == SQLITE_DONE)
		return false;
	else
		throw static_error(rv);
}

int backup::remaining() {
	return sqlite3_backup_remaining(handle);
}

int backup::pagecount() {
	return sqlite3_backup_pagecount(handle);
}

} // namespace sqlitepp

