
env = Environment()

env.Append(
      CXXFLAGS = ['-std=c++11', '-Wall', '-Wextra'],
   )

src = Split('''
	parameter.cpp
	column.cpp
	config.cpp
	context.cpp
	error.cpp
	statement.cpp
	connection.cpp
	sqxx.cpp
	blob.cpp
	backup.cpp
	function.cpp
	value.cpp
   ''')

env_lib = env.Clone()
lib = env_lib.Library('sqxx', src)
Default(lib)

Export('lib')


# tests, ...

env_use = env.Clone()
env_use.Append(
		CPPPATH = ['#'],
		LIBS = ['sqlite3'],
	)
Export('env_use')

SConscript(dirs = ['test', 'test-includes', 'examples'])

Alias('test', ['test_unit'])

