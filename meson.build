project(
  'equation',
  'c',
  version : '0.1.0',
  meson_version : '>=0.58',
  license : 'GPL-2.0-only',
  default_options : ['c_std=gnu11', 'warning_level=2'],
)
cc = meson.get_compiler('c')
equation_intl_library = cc.find_library('intl', required : false)
if not equation_intl_library.found()
  equation_intl_library = declare_dependency()
endif
equation_have_gettext = cc.links(
  '''
#include <libintl.h>
int main(void) {
  bindtextdomain("equation", "/tmp");
  bind_textdomain_codeset("equation", "UTF-8");
  return gettext("test") == 0;
}
''',
  dependencies : equation_intl_library,
  name : 'gettext support',
)
equation_build_config = configuration_data()
equation_build_config.set10('EQUATION_ENABLE_NLS', equation_have_gettext)
equation_build_config.set_quoted(
  'EQUATION_LOCALEDIR',
  get_option('prefix') / get_option('localedir'),
)
configure_file(
  output : 'equation_build_config.h',
  configuration : equation_build_config,
)
subdir('src/intl')
if equation_have_gettext
  subdir('po')
endif
elementary_dep = dependency('elementary', version : '>=1.26')
equation_settings_deps = [dependency('eet'), dependency('ecore-file')]
subdir('data')
subdir('src/config')
subdir('src/engine')
subdir('src/fe-efl')
subdir('tests')
if equation_have_gettext
  i18n.merge_file(
    input : 'data/equation_calculator.desktop',
    output : 'equation.desktop',
    type : 'desktop',
    po_dir : 'po',
    install : true,
    install_dir : get_option('datadir') / 'applications',
  )
else
  install_data(
    'data/equation_calculator.desktop',
    rename : 'equation.desktop',
    install_dir : get_option('datadir') / 'applications',
  )
endif
install_data(
  'data/equation_calculator.svg',
  install_dir : get_option('datadir') / 'icons/hicolor/scalable/apps',
)

install_data(
  'AUTHORS',
  'COPYING',
  install_dir : get_option('datadir') / 'doc/equation',
)

equation_python = find_program('python3')
run_target(
  'equation-format',
  command : [equation_python, files('tools/equation_format.py')],
)
run_target(
  'equation-format-check',
  command : [equation_python, files('tools/equation_format.py'), '--check'],
)
