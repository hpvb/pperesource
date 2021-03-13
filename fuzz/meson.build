fuzz = executable(
	'fuzz',
	[ 'fuzz.c', pperesource_sources],
	include_directories: inc,
)

fuzz_test = executable(
	'fuzz-test',
	[ 'fuzz-test.c', pperesource_sources],
	include_directories: inc,
)

test('fuzzer run', fuzz_test,
	timeout: 240,
	args: [meson.source_root() + '/fuzz/corpus', '-max_total_time=120']
)
