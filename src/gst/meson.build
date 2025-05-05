libparolegst_sources = [
  'parole-gst.c',
  'parole-gst.h',
  parole_enum_types_h,
]

libparolegst_sources += gnome.genmarshal(
  'gstmarshal',
  sources: 'gstmarshal.list',
  prefix: '_gmarshal',
  internal: true,
  install_header: false,
)

libparolegst_sources += gnome.mkenums_simple(
  'gst-enum-types',
  sources: [
    'parole-gst.h',
  ]
)

libparolegst_deps = [
  glib,
  gstreamer,
  gstreamer_base,
  gstreamer_video,
  gstreamer_audio,
  gstreamer_pbutils,
  gstreamer_tag,
  gtk,
  libparole_dep,
  libparolecommon_dep,
  libxfce4util,
  x11_deps,
]

libparolegst = static_library(
  'parolegst',
  libparolegst_sources,
  include_directories: [
    include_directories('..' / '..'),
    include_directories('..' / 'misc'),
  ],
  c_args: [
    '-DPAROLE_COMPILATION',
    '-DG_LOG_DOMAIN="@0@"'.format('parolegst'),
  ],
  dependencies: libparolegst_deps,
  install: false,
)

libparolegst_dep = declare_dependency(
  dependencies: libparolegst_deps,
  link_with: libparolegst,
)
