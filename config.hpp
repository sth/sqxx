
#if !defined(SQXX_CONFIG_HPP_INCLUDED)
#define SQXX_CONFIG_HPP_INCLUDED

#include <functional>

struct mem_malloc;
struct mem_methods;
struct mutex_methods;
struct pcache_methods2;

namespace sqxx {

void config_singlethread();
void config_multithread();
void config_serialized();
void config_malloc(mem_methods*);
void config_getmalloc(mem_methods*);
void config_memstatus(bool enable);
void config_scratch(void *buf, int sz, int n);
void config_pagecache(void *buf, int sz, int n);
void config_heap(void *buf, int bytes, int minalloc);
void config_mutex(mutex_methods*);
void config_getmutex(mutex_methods*);
void config_lookaside(int sz, int n);
void config_pcache2(pcache_methods2*);
void config_getpcache2(pcache_methods2*);
typedef std::function<void (int err, const char *msg)> log_handler_t;
void config_log(const log_handler_t &fun);
void config_log();
void config_uri(bool enable);
void config_covering_index_scan(bool enable);
typedef std::function<void (/*connection&,*/ const char *msg, int kind)> sqllog_handler_t;
void config_sqllog(const sqllog_handler_t &fun);
void config_sqllog();
void config_mmap_size(int64_t defaultlimit, int64_t maxlimit);

} // namespace sqxx

#endif // SQXX_CONFIG_HPP_INCLUDED

