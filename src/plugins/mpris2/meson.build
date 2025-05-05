parole_mpris2_sources = [
  'mpris2-plugin.c',
  'mpris2-provider.c',
  'mpris2-provider.h',
  parole_h,
]

shared_module(
  'parole-mpris2',
  parole_mpris2_sources,
  name_prefix: '',
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('mpris2-plugin'),
  ],
  include_directories: [
    include_directories('..' / '..' / '..'),
  ],
  dependencies: [
    glib,
    gtk,
    gstreamer,
    gstreamer_base,
    gstreamer_tag,
    libparole_dep,
    libxfce4util,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / 'parole-0',
)

i18n.merge_file(
  input: 'mpris2.desktop.in',
  output: 'mpris2.desktop',
  po_dir: '..' / '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: pkgdatadir / 'parole-plugins-0',
)
