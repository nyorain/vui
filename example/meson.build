dep_ny = dependency('ny', fallback: ['ny', 'ny_dep'])

example_src = [
	'example.cpp',
	'render.cpp',
	'window.cpp',
]

example_deps = [
	vui_dep,
	dep_rvg,
	dep_ny
]

executable('example',
	sources: example_src,
	dependencies: example_deps)
