equation_intl_lib = static_library(
  'equation_intl',
  'equation_intl.c',
  include_directories : include_directories('../..'),
  dependencies : equation_intl_library,
)
equation_intl_dep = declare_dependency(
  link_with : equation_intl_lib,
  include_directories : [
    include_directories('.'),
    include_directories('../..'),
  ],
  dependencies : equation_intl_library,
)
