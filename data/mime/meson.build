mime_type_include_sh = find_program('mime-type-include.sh')

parole_mime_types_h = configure_file(
  input: 'mime-type-list.txt',
  output: 'parole-mime-types.h',
  command: [mime_type_include_sh, '@INPUT@'],
  capture: true,
)
