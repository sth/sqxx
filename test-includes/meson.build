
# make sure all header files include everything they need

include_test_sources = [
		'inc_backup.cpp',
		'inc_blob.cpp',
		'inc_column.cpp',
		'inc_config.cpp',
		'inc_connection.cpp',
		'inc_context.cpp',
		'inc_error.cpp',
		'inc_global.cpp',
		'inc_parameter.cpp',
		'inc_sqxx.cpp',
		'inc_statement.cpp',
		'inc_value.cpp',
        'main.cpp',
    ]

executable('include_test',
        include_test_sources,
        include_directories: sqxx_include,
		link_with : sqxx,
	)

