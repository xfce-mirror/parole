parole_h = configure_file(
  input: 'parole.h.in',
  output: 'parole.h',
  configuration: {
    'PAROLE_VERSION_MAJOR': version_parts[0],
    'PAROLE_VERSION_MINOR': version_parts[1],
    'PAROLE_VERSION_MICRO': version_parts[2],
  }
)

parole_marshal_h = gnome.genmarshal(
  'parole-marshal',
  sources: 'parole-marshal.list',
  prefix: 'parole_marshal',
  internal: true,
  install_header: false,
)

parole_enum_types_h = gnome.mkenums_simple(
  'parole-enum-types',
  sources: [
    'parole-pl-parser.h',
    'parole-provider-player.h',
    'parole-stream.h',
  ]
)

libparole_sources = [
  'parole-debug.c',
  'parole-debug.h',
  'parole-file.c',
  'parole-file.h',
  'parole-filters.c',
  'parole-filters.h',
  'parole-pl-parser.c',
  'parole-pl-parser.h',
  'parole-provider-player.c',
  'parole-provider-player.h',
  'parole-provider-plugin.c',
  'parole-provider-plugin.h',
  'parole-stream.c',
  'parole-stream.h',
  parole_enum_types_h,
  parole_h,
  parole_marshal_h,
  parole_mime_types_h,
]

libparole_deps = [
  gio,
  glib,
  gobject,
  gtk,
  libxfce4util,
  taglib_c,
]

# We need a shared lib for gtk-doc.
libparole = both_libraries(
  'parole',
  libparole_sources,
  include_directories: [
    include_directories('..' / '..'),
  ],
  c_args: [
    '-DPAROLE_COMPILATION',
    '-DPIXMAPS_DIR="@0@"'.format(pkgdatadir / 'pixmaps'),
    '-DG_LOG_DOMAIN="@0@"'.format('parole'),
  ],
  dependencies: libparole_deps,
  install: false,
)

libparole_inc = include_directories('.')

libparole_dep = declare_dependency(
  dependencies: libparole_deps,
  link_with: libparole.get_static_lib(),
)

libparole_doc_dep = declare_dependency(
  dependencies: libparole_deps,
  link_with: libparole,
)

install_headers(
  'parole-debug.h',
  'parole-file.h',
  'parole-filters.h',
  'parole-pl-parser.h',
  'parole-provider-player.h',
  'parole-provider-plugin.h',
  'parole-stream.h',
  parole_h,
  subdir: 'parole',
)
