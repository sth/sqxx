
boost_test = dependency('boost', modules : ['test'])

sqxx_test = executable('sqxx_test',
		['sqxx_test.cpp', 'examples_test.cpp', 'driver.cpp'],
        include_directories: sqxx_include,
		link_with : sqxx,
		dependencies : [boost_test],
		cpp_args : ['-DBOOST_TEST_DYN_LINK'],
	)
test('sqxx api', sqxx_test)

