project('bril-interpreter',
        'cpp',
        default_options: [
          'cpp_std=c++23',
          'warning_level=3',  # similar to -Wall -Wextra
        ])

srcs = [
  'src/main.cpp',
  'src/interpreter.cpp'
  ]

sjp_proj = subproject('libsjp')
libsjp_dep = sjp_proj.get_variable('libsjp_dep')

main_exe = executable('main',
                      srcs,
                      dependencies: libsjp_dep)
