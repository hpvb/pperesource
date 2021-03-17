if cc.get_id() != 'msvc'
	if cc.get_argument_syntax() == 'gcc'
		extra_args = ['-fvisibility=hidden']
	else
		if cc.has_argument('-fvisibility=hidden')
			extra_args = ['-fvisibility=hidden']
		endif
	endif
else
	extra_args = []
endif

pperesource_sources = files([
	'main.c',
	'pe/data_directory.c',
	'pe/header_deserialize.c',
	'pe/header_print.c',
	'pe/header_serialize.c',
	'pe/section.c',
	'pe/section_deserialize.c',
	'pe/section_print.c',
	'pe/section_serialize.c',
	'ppe_error.c',
	'resources/resource_table.c',
	'resources/resource_table_deserialize.c',
	'resources/resource_table_print.c',
	'resources/resource_table_serialize.c',
	'resources/string_table.c',
	'resources/versioninfo.c',
	'resources/versioninfo_deserialize.c',
	'resources/versioninfo_serialize.c',
	'utils.c',
])

if host_machine.system() == 'windows'
	pperesource_sources += files('thirdparty/winiconv/win_iconv.c')
endif

pperesource = shared_library(
	'pperesource',
	pperesource_sources,
	c_args: extra_args,
	include_directories: inc,
	dependencies: libs,
	install: true,
	version: meson.project_version(),
	soversion: 0
)
