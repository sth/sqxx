
project('sqxx', 'cpp',
	default_options: ['cpp_std=c++14'])

sqlite3 = dependency('sqlite3')

sources = [
		'backup.cpp',
		'blob.cpp',
		'column.cpp',
		'config.cpp',
		'connection.cpp',
		'context.cpp',
		'error.cpp',
		'global.cpp',
		'parameter.cpp',
		'sqxx.cpp',
		'statement.cpp',
		'value.cpp',
	]

sqxx_include = include_directories('.')

sqxx = static_library('sqxx', sources,
	dependencies : sqlite3)
sqxx_so = shared_library('sqxx', sources,
	dependencies : sqlite3)

subdir('test')
subdir('test-includes')

