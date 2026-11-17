equation_ui_lib = static_library(
  'equation_ui',
  'equation_ui.c',
  'equation_window.c',
  'equation_theme.c',
  c_args : [
    '-DEQUATION_ICON_SOURCE="' + meson.project_source_root() / 'data/equation_calculator.svg' + '"',
    '-DEQUATION_ICON_INSTALL="' + get_option('prefix') / get_option('datadir') / 'icons/hicolor/scalable/apps/equation_calculator.svg' + '"',
    '-DEQUATION_VERSION="' + meson.project_version() + '"',
    '-DEQUATION_THEME_BUILD_DIR="' + meson.project_build_root() / 'data/themes' + '"',
    '-DEQUATION_BUILD_ROOT="' + meson.project_build_root() + '"',
    '-DEQUATION_THEME_INSTALL_DIR="' + get_option('prefix') / get_option(
      'datadir',
    ) / 'equation/themes' + '"',
  ],
  dependencies : [
    elementary_dep,
    equation_engine_dep,
    equation_config_dep,
    equation_intl_dep,
  ],
)
equation_ui_dep = declare_dependency(
  link_with : equation_ui_lib,
  include_directories : include_directories('.'),
  dependencies : [
    elementary_dep,
    equation_engine_dep,
    equation_config_dep,
    equation_intl_dep,
  ],
)
executable(
  'equation',
  'equation_main.c',
  dependencies : equation_ui_dep,
  link_depends : equation_themes,
  install : true,
)
