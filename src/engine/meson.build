equation_math_dep = meson.get_compiler('c').find_library('m')
equation_engine = static_library(
  'equation_engine',
  'equation_engine.c',
  dependencies : equation_intl_dep,
)
equation_engine_dep = declare_dependency(
  link_with : equation_engine,
  dependencies : [equation_math_dep, equation_intl_dep],
  include_directories : include_directories('.'),
)
