
env = Environment()

env.Append(
      CXXFLAGS = ['-std=c++11', '-Wall'],
   )

src = Split('''
	parameter.cpp
	column.cpp
	config.cpp
	context.cpp
	statement.cpp
	connection.cpp
	sqxx.cpp
	blob.cpp
	backup.cpp
	func.cpp
	value.cpp
   ''')

lib = env.Library('sqxx', src)

test_src = Split('''
	test/driver.cpp
   test/sqxx_test.cpp
   ''')


# tests

env_test = env.Clone()
env_test.Append(
		CPPPATH = '.',
		CPPDEFINES = ['BOOST_TEST_DYN_LINK'],
		LIBS = ['boost_unit_test_framework', 'sqlite3']
	)
env_test.Program('sqxx_test', test_src + [lib])

env_test.Command('runtest', 'sqxx_test', './sqxx_test')

Alias('test', ['sqxx_test', 'runtest'])


# examples

env_examples = env.Clone()
env_examples.Append(
		CPPPATH = ['.'],
		LIBS = ['sqlite3'],
	)

env_examples.Program('examples/usage', ['examples/usage.cpp', lib])

Alias('examples', ['examples/usage'])


Default(lib)

