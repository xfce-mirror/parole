libparolecommon_sources = [
  'parole-common.c',
  'parole-common.h',
  'parole-powermanager.c',
  'parole-powermanager.h',
  'parole-rc-utils.c',
  'parole-rc-utils.h',
]

libparolecommon_deps = [
  gio,
  glib,
  gtk,
  libxfce4util,
]

libparolecommon = static_library(
  'parolecommon',
  libparolecommon_sources,
  c_args: [
    '-DG_LOG_DOMAIN="@0@"'.format('parolecommon'),
  ],
  dependencies: libparolecommon_deps,
  install: false,
)

libparolecommon_dep = declare_dependency(
  dependencies: libparolecommon_deps,
  link_with: libparolecommon,
)

if enable_x11
  libparolescreensaver_sources = [
    'parole-screensaver.c',
    'parole-screensaver.h',
  ]

  libparolescreensaver_deps = [
    glib,
    gtk,
    libxfce4util,
    x11_deps,
  ]

  libparolescreensaver = static_library(
    'parolescreensaver',
    libparolescreensaver_sources,
    c_args: [
      '-DG_LOG_DOMAIN="@0@"'.format('parolescreensaver'),
    ],
    dependencies: libparolescreensaver_deps,
    install: false,
  )

  libparolescreensaver_dep = declare_dependency(
    dependencies: libparolescreensaver_deps,
    link_with: libparolescreensaver,
  )
else
  libparolescreensaver_dep = dependency('', required: false)
endif
