equation_test_catalog = custom_target(
  'equation test catalog',
  input : '../../../equation_locale.po',
  output : 'equation.mo',
  command : [
    find_program('msgfmt'),
    '--check-format',
    '-o',
    '@OUTPUT@',
    '@INPUT@',
  ],
)
