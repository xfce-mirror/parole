version_xml = configure_file(
  input: 'version.xml.in',
  output: 'version.xml',
  configuration: {
    'VERSION': meson.project_version(),
  },
)

gnome.gtkdoc(
  'Parole-Plugins',
  mode: 'sgml',
  main_xml: 'Parole-Plugins-docs.sgml',
  src_dir: libparole_inc,
  content_files: [
    version_xml,
    'parole-plugins-tut.sgml',
  ],
  scan_args: '--rebuild-types',
  dependencies: libparole_doc_dep,
  ignore_headers: [
    'parole-enum-types.h',
    'parole-marshal.h',
  ],
  install: true,
)
