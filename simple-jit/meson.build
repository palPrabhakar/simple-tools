project(
  'simple-jit',
  'cpp',
  version : '0.1',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',  # similar to -Wall -Wextra
  ]
)

# Add stricter warnings (like your -Wpedantic etc.)
add_project_arguments(
  '-Wpedantic', '-Wconversion', '-Wswitch', '-ggdb',
  language : 'cpp'
)

# The library
sjp_proj = subproject('libsjp')
libsjp_dep = sjp_proj.get_variable('libsjp_dep')

srcs = [
  'src/main.cpp',
  'src/parser.cpp',
  'src/parser.h',
  'src/emitter.cpp',
  'src/emitter.h',
  ]

# Main executable (only built if this is the main project)
main_exe = executable(
  'main',
  srcs,
  dependencies: libsjp_dep,
)

