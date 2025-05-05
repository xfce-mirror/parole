sample_plugin_sources = [
  'sample-plugin.c',
  'sample-provider.c',
  'sample-provider.h',
  parole_h,
]

shared_module(
  'sample-plugin',
  sample_plugin_sources,
  name_prefix: '',
  gnu_symbol_visibility: 'hidden',
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('sample-plugin'),
  ],
  include_directories: [
    include_directories('..' / '..' / '..'),
  ],
  dependencies: [
    glib,
    gtk,
    libparole_dep,
    libxfce4util,
  ],
  install: false,
)
