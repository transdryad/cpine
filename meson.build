project(
  'cpine',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++23'],
)

dependencies = [
]

exe = executable(
  'cpine',
  'src/cpine.cpp',
  'src/server.hpp',
  'src/server.cpp',
  'src/client.cpp',
  'src/client.hpp',
  'src/types.hpp',
  'src/types.cpp',
  install : true,
  dependencies : dependencies,
)

test('basic', exe)
