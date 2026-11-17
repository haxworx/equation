equation_config_dep = declare_dependency(
  link_with : static_library(
    'equation_config',
    'equation_config.c',
    dependencies : equation_settings_deps,
  ),
  include_directories : include_directories('.'),
  dependencies : equation_settings_deps,
)
