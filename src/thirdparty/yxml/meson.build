extra_cargs = []

if cc.get_argument_syntax() == 'gcc'
	extra_cargs = ['-Wno-conversion']
endif

yxml = static_library(
	'yxml',
	'yxml.c',
	override_options: ['warning_level=0', 'werror=false'],
	c_args: extra_cargs,
	gnu_symbol_visibility: 'hidden',
	install: false
)