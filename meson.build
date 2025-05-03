project(
  'parole',
  'c',
  version: '4.18.2-dev',
  license: 'GPL-2.0-or-later',
  meson_version: '>= 0.56.0',
  default_options: [
    'c_std=gnu11',
    'buildtype=debugoptimized',
    'warning_level=2',
  ]
)

project_namespace = 'apps'
pkgdatadir = get_option('prefix') / get_option('datadir') / meson.project_name()

cc = meson.get_compiler('c')
pkgconfig = import('pkgconfig')
gnome = import('gnome')
i18n = import('i18n')

dependency_versions = {
  'glib': '>= 2.38.0',
  'gtk': '>= 3.22.0',
  'dbus': '>= 0.60',
  'dbus-glib': '>= 0.70',
  'xfce4': '>= 4.16.0',
  'libnotify': '>= 0.7.8',
  'x11': '>= 1.6.7',
  'gstreamer': '>= 1.0.0',
  'taglib': '>= 1.4',
}

glib = dependency('glib-2.0', version: dependency_versions['glib'])
gobject = dependency('gobject-2.0', version: dependency_versions['glib'])
gmodule = dependency('gmodule-2.0', version: dependency_versions['glib'])
gio = dependency('gio-2.0', version: dependency_versions['glib'])
gtk = dependency('gtk+-3.0', version: dependency_versions['gtk'])
dbus = dependency('dbus-1', version: dependency_versions['dbus'])
dbus_glib = dependency('dbus-glib-1', version: dependency_versions['dbus-glib'])
libxfce4ui = dependency('libxfce4ui-2', version: dependency_versions['xfce4'])
libxfce4util = dependency('libxfce4util-1.0', version: dependency_versions['xfce4'])
xfconf = dependency('libxfconf-0', version: dependency_versions['xfce4'])

gstreamer = dependency('gstreamer-1.0', version: dependency_versions['gstreamer'])
gstreamer_base = dependency('gstreamer-base-1.0', version: dependency_versions['gstreamer'])
gstreamer_video = dependency('gstreamer-video-1.0', version: dependency_versions['gstreamer'])
gstreamer_audio = dependency('gstreamer-audio-1.0', version: dependency_versions['gstreamer'])
gstreamer_pbutils = dependency('gstreamer-pbutils-1.0', version: dependency_versions['gstreamer'])
gstreamer_tag = dependency('gstreamer-tag-1.0', version: dependency_versions['gstreamer'])

x11_deps = []
x11_deps += dependency('gdk-x11-3.0', version: dependency_versions['gtk'], required: get_option('x11'))
x11_deps += dependency('x11', version: dependency_versions['x11'], required: get_option('x11'))

wayland_deps = []
wayland_deps += dependency('gdk-wayland-3.0', version: dependency_versions['gtk'], required: get_option('wayland'))

taglib_c = dependency('taglib_c', version: dependency_versions['taglib'], required: get_option('taglib'))
libnotify = dependency('libnotify', version: dependency_versions['libnotify'], required: get_option('notify-plugin'))

dbus_binding_tool = find_program('dbus-binding-tool')

feature_cflags = []

enable_mpris2 = not get_option('mpris2-plugin').disabled()
enable_tray = not get_option('tray-plugin').disabled()
enable_notify = libnotify.found()

if cc.has_function('strcoll')
  feature_cflags += '-DHAVE_STRCOLL=1'
endif
if taglib_c.found()
  feature_cflags += '-DHAVE_TAGLIBC=1'
endif

enable_x11 = not get_option('x11').disabled()
foreach dep : x11_deps
  enable_x11 = enable_x11 and dep.found()
endforeach
if enable_x11
  feature_cflags += '-DENABLE_X11=1'
  if cc.has_header_symbol('X11/XF86keysym.h', 'XF86XK_AudioPlay')
    feature_cflags += '-DHAVE_XF86_KEYSYM=1'
  endif
endif

enable_wayland = not get_option('wayland').disabled()
foreach dep : wayland_deps
  enable_wayland = enable_wayland and dep.found()
endforeach
if enable_wayland
  feature_cflags += '-DENABLE_WAYLAND=1'
endif

if not enable_x11 and not enable_wayland
  error('At least one of the X11 and Wayland backends must be enabled')
endif

extra_cflags = []
extra_cflags_check = [
  '-Wmissing-declarations',
  '-Wmissing-noreturn',
  '-Wold-style-definition',
  '-Wredundant-decls',
  '-Wpointer-arith',
  '-Wcast-align',
  '-Winit-self',
  '-Wshadow',
  '-Wmissing-include-dirs',
  '-Wundef',
  '-Wformat',
  '-Wformat-security',
  '-Wformat-y2k',
  '-Wnested-externs',
  '-Wno-unused-parameter',
  '-Wno-declaration-after-statement',
  '-Wno-missing-field-initializers',
  '-Werror=implicit-function-declaration',
  '-Wno-error=deprecated-declarations',
]

optimization = get_option('optimization')
if get_option('debug') and optimization in ['0', 'g']
  extra_cflags_check += '-fstack-protector-strong'
  extra_cflags += [
    '-DDEBUG=1',
    '-DDEBUG_TRACE=1',
    '-DG_ENABLE_DEBUG',
  ]
elif optimization in ['3', 'minsize']
  extra_cflags += [
    '-DNDEBUG',
    '-DG_DISABLE_CAST_CHECKS',
    '-DG_DISABLE_ASSERT',
  ]
endif

if dependency_versions.has_key('glib')
  glib_version_parts = dependency_versions['glib'].split(' ')
  glib_min_version_parts = glib_version_parts[1].split('.')
  glib_min_version_define = 'GLIB_VERSION_@0@_@1@'.format(glib_min_version_parts[0], glib_min_version_parts[1])
  extra_cflags += [
    '-DGLIB_VERSION_MIN_REQUIRED=@0@'.format(glib_min_version_define),
    '-DGLIB_VERSION_MAX_ALLOWED=@0@'.format(glib_min_version_define),
    '-DG_LOG_USE_STRUCTURED=1',
  ]
endif

version_parts = meson.project_version().split('-dev')[0].split('.')
version_short = '@0@.@1@'.format(version_parts[0], version_parts[1])

extra_cflags += [
  '-DPACKAGE="@0@"'.format(meson.project_name()),
  '-DPACKAGE_NAME="@0@"'.format(meson.project_name()),
  '-DPACKAGE_VERSION="@0@"'.format(meson.project_version()),
  '-DVERSION="@0@"'.format(meson.project_version()),
  '-DVERSION_SHORT="@0@"'.format(version_short),
  '-DPACKAGE_STRING="@0@ @1@"'.format(meson.project_name(), meson.project_version()),
  '-DPACKAGE_DATADIR="@0@"'.format(pkgdatadir),
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DPACKAGE_URL="https://docs.xfce.org/@0@/@1@/start"'.format(project_namespace, meson.project_name()),
  '-DPACKAGE_BUGREPORT="https://gitlab.xfce.org/@0@/@1@/-/issues"'.format(project_namespace, meson.project_name()),
  '-DGETTEXT_PACKAGE="@0@"'.format(meson.project_name()),
  '-DPREFIX="@0@"'.format(get_option('prefix')),
  '-DBINDIR="@0@"'.format(get_option('prefix') / get_option('bindir')),
  '-DDATADIR="@0@"'.format(get_option('prefix') / get_option('datadir')),
  '-DDOCDIR="@0@"'.format(get_option('prefix') / 'doc' / meson.project_name()),
  '-DINCLUDEDIR="@0@"'.format(get_option('prefix') / get_option('includedir')),
  '-DLIBDIR="@0@"'.format(get_option('prefix') / get_option('libdir')),
  '-DLIBEXECDIR="@0@"'.format(get_option('prefix') / get_option('libexecdir')),
  '-DLOCALEDIR="@0@"'.format(get_option('prefix') / get_option('localedir')),
  '-DLOCALSTATEDIR="@0@"'.format(get_option('prefix') / get_option('localstatedir')),
  '-DSBINDIR="@0@"'.format(get_option('prefix') / get_option('sbindir')),
  '-DSYSCONFDIR="@0@"'.format(get_option('prefix') / get_option('sysconfdir')),
  '-DHAVE_XFCE_REVISION_H=1',
]

add_project_arguments(cc.get_supported_arguments(extra_cflags_check), language: 'c')
add_project_arguments(feature_cflags, language: 'c')
add_project_arguments(extra_cflags, language: 'c')

xfce_revision_h = vcs_tag(
  command: ['git', 'rev-parse', '--short', 'HEAD'],
  fallback: 'UNKNOWN',
  input: 'xfce-revision.h.in',
  output: 'xfce-revision.h',
  replace_string: '@REVISION@',
)

subdir('data')
subdir('po')
subdir('src')
if get_option('gtk-doc')
  subdir('docs')
endif
