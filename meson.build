project(
	'pperesource', 'c',
	default_options: [
		'c_std=c11',
		'warning_level=2',
	],
	version: '0.0.1',
	license: 'Apache-2.0',
)

cc = meson.get_compiler('c')

if get_option('use_clang_fuzzer') == true
	if cc.get_id() != 'clang'
		error('use_clang_fuzzer must use CC=clang')
	endif

	add_project_arguments(
		['-fsanitize=fuzzer-no-link,address,undefined', '-DFUZZ=1'],
		language: ['c', 'cpp'],
	)
	add_project_link_arguments(
		['-fsanitize=fuzzer,address,undefined', '-Wl,--unresolved-symbols=ignore-in-object-files'],
		language: ['c', 'cpp'],
	)
endif

if cc.get_argument_syntax() == 'gcc'
	add_project_arguments(
		['-Wconversion', '-D_POSIX_C_SOURCE=200809L'],
		language: ['c', 'cpp'],
	)
endif

if cc.get_id() == 'msvc'
	add_project_arguments(
		'-D_CRT_SECURE_NO_WARNINGS=1',
		language: ['c', 'cpp'],
	)
endif

m_dep = cc.find_library('m', required : false)
libs = [m_dep]

if host_machine.system() == 'windows'
	inc = include_directories('include', 'src', 'src/thirdparty/lodepng', 'src/thirdparty/winiconv')
else
	inc = include_directories('include', 'src', 'src/thirdparty/lodepng')

	if cc.has_function('iconv_open')
		found_iconv = true
	elif cc.has_header_symbol('iconv.h', 'iconv_open')
		libs += declare_dependency(link_args : ['-liconv'])
	else
		error('Cannot find iconv')
	endif
endif

subdir('include')
subdir('src')

if get_option('use_clang_fuzzer') == false
	subdir('test')
else
	subdir('fuzz')
endif

pkg = import('pkgconfig')
pkg.generate(
	pperesource,
	description: 'Portable library for manipulating PE resources',
)
