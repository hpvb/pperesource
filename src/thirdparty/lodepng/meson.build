lodepng_extra_cargs = ['-DLODEPNG_NO_COMPILE_DISK=1']

if cc.get_argument_syntax() == 'gcc'
	lodepng_extra_cargs += ['-Wno-conversion']
endif

lodepng = static_library(
	'lodepng',
	'lodepng.c',
	override_options: ['warning_level=3', 'werror=false'],
	c_args: lodepng_extra_cargs,
	gnu_symbol_visibility: 'hidden',
	install: false
)