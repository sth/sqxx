
// (c) 2013 Stephan Hohe

#if !defined(STHCXX_SQLITEPP_INCLUDED)
#define STHCXX_SQLITEPP_INCLUDED

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>

// structs from <sqlite3.h>
struct sqlite3;
struct sqlite3_stmt;

namespace sqlitepp {

class error : public std::runtime_error {
public:
	int code;
	error(int code_arg, const char *what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
	error(int code_arg, const std::string &what_arg) : std::runtime_error(what_arg), code(code_arg) {
	}
};


typedef std::pair<const void*, int> blob;

inline blob make_blob(const void *data, int len) {
	return std::make_pair(data, len);
}

template<typename T>
inline blob make_blob(const std::vector<T> &data) {
	return vector_blob(data);
}

template<typename T>
typename std::enable_if<
	std::is_pod<T>::value,
	std::vector<T>
>::type blob_vector(const blob &b) {
	const T *begin = reinterpret_cast<const T*>(b.first);
	const T *end = begin + (b.second / sizeof(T));
	return std::vector<T>(begin, end);
}

template<typename T>
typename std::enable_if<
	std::is_pod<T>::value,
	blob
>::type vector_blob(const std::vector<T> &v) {
	return std::make_pair(v.data(), v.size() * sizeof(T));
}

/*
template<typename T>
typename std::enable_if<std::is_pod<T>::value, std::vector<T>>::type to_vector(const blob &b) {
	const T *begin = reinterpret_cast<const T*>(b.first);
	const T *end = begin + (b.second / sizeof(T));
	return std::vector<T>(begin, end);
}
*/

class statement;

class parameter {
private:
	statement &s;
	int idx;

public:
	parameter(statement &s, int idx);

	const char* name();

	void bind(int value);
	void bind(int64_t value);
	void bind(double value);
	void bind(const char *value, bool copy = true);
	void bind(const std::string &value, bool copy = true) { bind(value.c_str(), copy); }
	void bind(const blob &value, bool copy = true);
	template<typename T>
	void bind(const std::vector<T> &value, bool copy = true) { bind(vector_blob(value), copy); }
	void bind_zeroblob(int len);
};


class column {
private:
	statement &s;
	int idx;

public:
	column(statement &s, int idx);

	const char* name() const;
	const char* database_name() const;
	const char* table_name() const;
	const char* origin_name() const;

	int type() const;
	const char* decl_type() const;

	template<typename T>
	T val() const;
};

template<> int column::val<int>() const;
template<> int64_t column::val<int64_t>() const;
template<> double column::val<double>() const;
template<> const char* column::val<const char*>() const;
template<> inline std::string column::val<std::string>() const { return val<const char*>(); }
template<> blob column::val<blob>() const;
/* No partial specialization of function templates
template<typename T>
std::vector<T> column::val<std::vector<T>>() const {
	return blob_vector<T>(val<blob>());
}
*/

struct hook_table;

struct column_metadata {
	const char *datatype;
	const char *collseq;
	bool notnull;
	bool primarykey;
	bool autoinc;
};

class connection {
private:
	sqlite3 *handle;

	std::unique_ptr<hook_table> hook_tab;

	friend class hook_table;

public:
	connection();
	explicit connection(const char *filename, int flags = 0);
	explicit connection(const std::string &filename, int flags = 0);
	~connection() noexcept;

	// Don't copy, move
	connection(const connection&) = delete;
	connection& operator=(const connection&) = delete;
	connection(connection&&) = default;
	connection& operator=(connection&&) = default;

	/** sqlite3_db_filename() */
	const char* filename(const char *db = "main") const;
	const char* filename(const std::string &db) const { return filename(db.c_str()); }
	/** sqlite3_db_readonly() */
	bool readonly(const char *dbname) const;
	bool readonly(const std::string &dbname) const { return readonly(dbname.c_str()); }
	/** sqlite3_db_status() */
	std::pair<int, int> status(int op, bool reset=false);
	/** sqlite3_table_column_metadata() */
	column_metadata metadata(const char *db, const char *table, const char *column) const;
	/** sqlite3_total_changes() */
	int total_changes() const;

	void open(const char *filename, int flags = 0);
	void open(const std::string &filename, int flags = 0) { open(filename.c_str(), flags); }
	void close() noexcept;
	void close_sync();

	void create_collation(const char *name, const std::function<int (size_t, const char*, size_t, const char*)> &coll);

	/** Create a sql statement */
	statement prepare(const char *sql);
	//void exec(const char *sql, const std::function<void ()> &callback);
	/** Execute sql, discarding any results */
	void exec(const char *sql);

	void interrupt();
	int limit(int id, int newValue);

	hook_table& hooks();

	void release_memory();

	sqlite3* raw() { return handle; }
};


class result;

class statement {
private:
	sqlite3_stmt *handle;

	explicit statement(sqlite3_stmt *a_handle);

	friend class connection;
	friend class column;
	friend class parameter;
	//friend class result;

public:
	~statement();

	const char* sql();
	/** sqlite3_stmt_status() */
	int status(int op, bool reset=false);

	// Don't copy, move
	statement(const statement &) = delete;
	statement& operator=(const statement&) = delete;
	statement(statement &&) = default;
	statement& operator=(statement&&) = default;

	int param_index(const char *name) const;
	int param_index(const std::string &name) const { return param_index(name.c_str()); }
	int param_count() const;

	parameter param(int idx);
	parameter param(const char *name);
	parameter param(const std::string &name) { return param(name.c_str()); }

	int col_count() const;
	column col(int idx);

	bool step();
	void exec();
	// iterator/range access to all result rows
	result run();

	//column col(const char *name);

	//int changes();

	void reset();
	void clear_bindings();

	sqlite3_stmt* raw() { return handle; }
};


class result {
private:
	statement &s;

	explicit result(statement &a_s) : s(a_s) {
	};

public:
	class iterator : public std::iterator<std::input_iterator_tag, result> {
	private:
		result *r;
		size_t pos;

	public:
		explicit iterator(result *a_r = nullptr) : r(a_r), pos((size_t)-1) {
			if (a_r) {
				// advance to first result element
				++(*this);
			}
		}

		result& operator*() const {
			return *r;
		}

		iterator& operator++() {
			if (r->next())
				++pos;
			else
				pos = (size_t)-1;
			return *this;
		}
		// Not reasonable implementable:
		//operator++(int);

		bool operator==(const iterator &other) const {
			return (pos == other.pos);
		}
		bool operator!=(const iterator &other) const {
			return !(*this == other);
		}
	};

	int col_count() const {
		return s.col_count();
	}
	column col(int idx) {
		return s.col(idx);
	}

	bool next() {
		return s.step();
	}

	iterator begin() {
		return iterator(this);
	}

	iterator end() {
		return iterator();
	}

	int changes() const;
};

} // namespace sqlitepp

#endif // STHCXX_SQLITEPP_INCLUDED

