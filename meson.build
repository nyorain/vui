project('vui', ['cpp', 'c'],
	license: 'BSL',
	version: '0.1.0',
	meson_version: '>=0.43',
	default_options: [
		'cpp_std=c++1z',
		'c_std=c99',
  		'warning_level=3',
  		'werror=true'])

build_example = get_option('examples')
build_tests = get_option('tests')

warnings = [
	'-Wall',
	'-Wextra',
	'-Wpedantic',

	# extra
	'-Wduplicated-cond',
	'-Wrestrict',
	'-Wnull-dereference',
	'-Wundef',
	'-Wcast-align=strict',
	'-Wlogical-op',

	# TODO: currently only for ny
	'-Wno-non-virtual-dtor',
]

# default arrguments
# warnings and stuff
cc = meson.get_compiler('cpp')
add_project_arguments(
	cc.get_supported_arguments(warnings),
	language: 'cpp')

# project-specific stuff
source_root = meson.source_root().split('\\')
add_project_arguments('-DDLG_BASE_PATH="' + '/'.join(source_root) + '/"', language: 'cpp')

dep_nytl = dependency('nytl MASTER', fallback: ['nytl', 'nytl_dep'])
dep_dlg = dependency('dlg', fallback: ['dlg', 'dlg_dep'])
dep_rvg = dependency('rvg', fallback: ['rvg', 'rvg_dep'])

src_inc = include_directories('src')
vui_inc = include_directories('include')

vui_deps = [
  dep_nytl,
  dep_dlg,
  dep_rvg,
]

subdir('src/vui')

vui_dep = declare_dependency(
	link_with: vui_lib,
	dependencies: vui_deps,
	include_directories: vui_inc)

if build_example
  subdir('example')
endif

if build_tests
  subdir('docs/tests')
endif

# pkgconfig
pkg = import('pkgconfig')
pkg_dirs = ['.']
pkg.generate(
	name: 'vui',
	requires: ['rvg'],
	libraries: vui_lib,
	filebase: 'vui',
	subdirs: pkg_dirs,
	version: meson.project_version(),
	description: 'Vulkan gui library')
