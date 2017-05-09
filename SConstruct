
env = Environment()

env.Append(
      CXXFLAGS = ['-std=c++14', '-Wall', '-Wextra'],
   )

src = Split('''
	parameter.cpp
	column.cpp
	config.cpp
	context.cpp
	error.cpp
	global.cpp
	statement.cpp
	connection.cpp
	sqxx.cpp
	blob.cpp
	backup.cpp
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
Alias('alltests', ['test_unit', 'test_inc'])

