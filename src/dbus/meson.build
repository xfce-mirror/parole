libparoledbus_sources = [
  'parole-dbus.c',
  'parole-dbus.h',
]

libparoledbus_deps = [
  dbus,
  dbus_glib,
  glib,
]

libparoledbus = static_library(
  'paroledbus',
  libparoledbus_sources,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('paroledbus'),
  ],
  dependencies: libparoledbus_deps,
  install: false,
)

libparoledbus_dep = declare_dependency(
  dependencies: libparoledbus_deps,
  link_with: libparoledbus,
)
