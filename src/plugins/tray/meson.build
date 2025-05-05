tray_icon_sources = [
  'tray-plugin.c',
  'tray-provider.c',
  'tray-provider.h',
  parole_h,
]

shared_module(
  'tray-icon',
  tray_icon_sources,
  name_prefix: '',
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('tray-plugin'),
  ],
  include_directories: [
    include_directories('..' / '..' / '..'),
  ],
  dependencies: [
    glib,
    gtk,
    libparole_dep,
    libxfce4ui,
    libxfce4util,
    xfconf,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / 'parole-0',
)

i18n.merge_file(
  input: 'system-tray.desktop.in',
  output: 'system-tray.desktop',
  po_dir: '..' / '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: pkgdatadir / 'parole-plugins-0',
)
