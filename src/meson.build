subdir('misc')
subdir('common')
subdir('gst')
subdir('dbus')
subdir('plugins')

parole_sources = [
  'main.c',
  'parole-about.c',
  'parole-about.h',
  'parole-builder.c',
  'parole-builder.h',
  'parole-button.c',
  'parole-button.h',
  'parole-conf.c',
  'parole-conf-dialog.c',
  'parole-conf-dialog.h',
  'parole-conf.h',
  'parole-disc.c',
  'parole-disc.h',
  'parole-mediachooser.c',
  'parole-mediachooser.h',
  'parole-medialist.c',
  'parole-medialist.h',
  'parole-module.c',
  'parole-module.h',
  'parole-open-location.c',
  'parole-open-location.h',
  'parole-player.c',
  'parole-player.h',
  'parole-plugin-player.c',
  'parole-plugin-player.h',
  'parole-plugins-manager.c',
  'parole-plugins-manager.h',
  'parole-shortcuts.c',
  'parole-shortcuts.h',
  'parole-subtitle-encoding.c',
  'parole-subtitle-encoding.h',
  'parole-utils.c',
  'parole-utils.h',
  'parole-vis.c',
  'parole-vis.h',
  parole_h,
  parole_resources,
  xfce_revision_h,
]

parole_sources += gnome.genmarshal(
  'gmarshal',
  sources: 'gmarshal.list',
  prefix: '_gmarshal',
  internal: true,
  install_header: false,
)

parole_sources += gnome.mkenums_simple(
  'enum-gtypes',
  sources: [
    'parole-button.h',
    'parole-conf.h',
  ]
)

parole_sources += configure_file(
  input: 'org.parole.media.list.xml',
  output: 'org.parole.media.list.h',
  command: [dbus_binding_tool, '--mode=glib-server', '--prefix=parole_media_list', '--output=@OUTPUT@', '@INPUT@'],
)

parole_sources += configure_file(
  input: 'org.parole.media.player.xml',
  output: 'org.parole.media.player.h',
  command: [dbus_binding_tool, '--mode=glib-server', '--prefix=parole_player', '--output=@OUTPUT@', '@INPUT@'],
)

executable(
  'parole',
  parole_sources,
  include_directories: [
    include_directories('..'),
  ],
  dependencies: [ 
    dbus,
    dbus_glib,
    gio,
    glib,
    gmodule,
    gobject,
    gstreamer,
    gtk,
    libparolecommon_dep,
    libparoledbus_dep,
    libparolegst_dep,
    libparolescreensaver_dep,
    libparole_dep,
    libxfce4ui,
    libxfce4util,
    taglib_c,
    x11_deps,
    xfconf,
  ],
  c_args: [
    '-DDESKTOPDIR="@0@"'.format(get_option('prefix') / get_option('datadir') / 'applications'),
    '-DPAROLE_PLUGINS_DIR="@0@"'.format(get_option('prefix') / get_option('libdir') / 'parole-0'),
    '-DPAROLE_PLUGINS_DATA_DIR="@0@"'.format(pkgdatadir / 'parole-plugins-0'),
    '-DPIXMAPS_DIR="@0@"'.format(pkgdatadir / 'pixmaps'),
    '-DPAROLE_COMPILATION',
    '-DG_LOG_DOMAIN="@0@"'.format('parole'),
  ],
  install: true,
  install_dir: get_option('prefix') / get_option('bindir'),
)
