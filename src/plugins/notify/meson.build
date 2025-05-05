parole_notify_sources = [
  'notify-plugin.c',
  'notify-provider.c',
  'notify-provider.h',
  parole_h,
]

shared_module(
  'parole-notify',
  parole_notify_sources,
  name_prefix: '',
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('notify-plugin'),
  ],
  include_directories: [
    include_directories('..' / '..' / '..'),
  ],
  dependencies: [
    glib,
    gtk,
    libnotify,
    libparole_dep,
    libxfce4util,
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('libdir') / 'parole-0',
)

i18n.merge_file(
  input: 'notify.desktop.in',
  output: 'notify.desktop',
  po_dir: '..' / '..' / '..' / 'po',
  type: 'desktop',
  install: true,
  install_dir: pkgdatadir / 'parole-plugins-0',
)
