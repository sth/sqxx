
# Compile out examples to make sure they actually work

example_usage = executable('usage',
	['usage.cpp'],
	include_directories : sqxx_include,
	link_with : sqxx,
)


# Better also run it as a test to make sure it at least doesn't crash/...
test('usage example', example_usage)

