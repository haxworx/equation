equation_engine_test = executable(
  'equation_engine_test',
  'equation_engine_test.c',
  dependencies : equation_engine_dep,
)
test('equation engine', equation_engine_test)
equation_config_test = executable(
  'equation_config_test',
  'equation_config_test.c',
  dependencies : equation_config_dep,
)
test('Eet preferences round trip and validation', equation_config_test)
equation_theme_test = executable(
  'equation_theme_test',
  'equation_theme_test.c',
  dependencies : elementary_dep,
)
test(
  'all theme groups and frame geometry',
  equation_theme_test,
  args : equation_themes,
  env : ['EINA_LOG_LEVEL=0', 'ECORE_NO_SYSTEM_MODULES=1'],
)
equation_ui_test = executable(
  'equation_ui_test',
  'equation_ui_test.c',
  dependencies : equation_ui_dep,
  link_depends : equation_themes,
)
foreach scale : ['1', '1.5', '2']
  test(
    'theme switching and calculator at scale ' + scale,
    equation_ui_test,
    env : [
      'ELM_ENGINE=buffer',
      'ELM_SCALE=' + scale,
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1',
    ],
  )
endforeach

if equation_have_gettext
  subdir('locale/zz/LC_MESSAGES')
  equation_locale_dir = meson.current_build_dir() / 'locale'
  equation_locale_test = executable(
    'equation_locale_test',
    'equation_locale_test.c',
    dependencies : equation_engine_dep,
  )
  test(
    'gettext catalog and stable numeric locale',
    equation_locale_test,
    args : [equation_locale_dir],
    depends : equation_test_catalog,
    env : ['LANGUAGE=zz', 'LC_ALL=en_US.UTF-8'],
  )
  test(
    'translated UI and calculator tokens',
    equation_ui_test,
    depends : equation_test_catalog,
    env : [
      'LANGUAGE=zz',
      'LC_ALL=en_US.UTF-8',
      'EQUATION_TEST_LOCALEDIR=' + equation_locale_dir,
      'ELM_ENGINE=buffer',
      'ELM_SCALE=1.5',
      'EINA_LOG_LEVEL=0',
      'ECORE_NO_SYSTEM_MODULES=1',
    ],
  )
endif
