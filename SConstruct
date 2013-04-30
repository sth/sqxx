
env = Environment()

env.Append(
      CXXFLAGS = ['-std=c++11', '-Wall']
   )

import buildsys
buildsys.setup(env)
env.BConfigure()


src = Split('''
	sqlitepp.cpp
	sqlitepp_blob.cpp
	sqlitepp_backup.cpp
	sqlitepp_func.cpp
   ''')

lib = env.Library('sqxx', src)

test_src = Split('''
	test/driver.cpp
   test/sqxx_test.cpp
   ''')

env_test = env.Clone()
env_test.Append(
		CPPPATH = '.',
		CPPDEFINES = ['BOOST_TEST_DYN_LINK'],
		LIBS = ['boost_unit_test_framework', 'sqlite3']
	)
env_test.Program('sqxx_test', test_src + [lib])

