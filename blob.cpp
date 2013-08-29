
// (c) 2013 Stephan Hohe

#include "sqxx.hpp"
#include "blob.hpp"
#include "detail.hpp"
#include <sqlite3.h>

namespace sqxx {

blob_source::blob_source(sqlite3_blob *handle_arg)
	: handle(handle_arg), pos(0), len(sqlite3_blob_bytes(handle)) {
}

std::streamsize blob_source::read(char *s, std::streamsize n) {
	int rv;
	// TODO: throw or fail() on too large n?!
	n = std::min(n, static_cast<std::streamsize>(std::numeric_limits<int>::max()));
	int limit = std::min(static_cast<int>(n), len-pos);
	rv = sqlite3_blob_read(handle, s, limit, pos);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	pos += limit;
	return limit;
}

std::streamsize blob_source::write(const char *s, std::streamsize n) {
	int rv;
	int limit = n;
	rv = sqlite3_blob_write(handle, s, limit, pos);
	if (rv != SQLITE_OK)
		throw static_error(rv);
	pos += limit;
	return limit;
}

std::streampos blob_source::seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way) {
	int next;
	if (way == std::ios_base::beg) {
		next = off;
	}
	else if (way == std::ios_base::cur) {
		next = pos + off;
	}
	else if (way == std::ios_base::end) {
		next = len + off - 1;
	}
	else {
		throw std::ios_base::failure("bad seek direction");
	}

	if (next < 0 || next >= len)
		throw std::ios_base::failure("bad seek offset");

	pos = next;
	return pos;
}

void blob_source::close() {
	sqlite3_blob_close(handle);
}

} // namepace sqxx

