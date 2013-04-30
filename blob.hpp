
// (c) 2013 Stephan Hohe

#if !defined(SQXX_BLOB_INCLUDED_HPP)
#define SQXX_BLOB_INCLUDED_HPP

#include <iosfwd>
#include <boost/iostreams/traits.hpp>
#include <boost/iostreams/categories.hpp>
#include <boost/iostreams/positioning.hpp>

struct sqlite3_blob;

namespace sqxx {

class blob_source {
private:
	sqlite3_blob *handle;
	int pos;
	int len;

	blob_source(sqlite3_blob *handle_arg);
	friend class connection;

public:
	blob_source(const blob_source&) = delete;
	blob_source& operator=(const blob_source&) = delete;
	blob_source(blob_source&&) = default;
	blob_source& operator=(blob_source&&) = default;

	typedef char char_type;
	struct category : boost::iostreams::seekable, boost::iostreams::closable_tag {
	};

	std::streamsize read(char *s, std::streamsize n);
	std::streamsize write(const char *s, std::streamsize n);
	std::streampos seek(boost::iostreams::stream_offset off, std::ios_base::seekdir way);
	void close();
};

} // namespace sqxx

#endif // SQXX_BLOB_INCLUDED_HPP

