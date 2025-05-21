m4_define([parole_verinfo],           [0:8:1])
m4_define([parole_version_api],       [0])
XDT_VERSION_INIT([4.20.0], [git])
m4_define([copyright_year], [2025])

AC_COPYRIGHT([Copyright (c) 2009-copyright_year() The Xfce development team])
AC_INIT([parole], [xdt_version], [https://gitlab.xfce.org/apps/parole])
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])
AC_DEFINE([VERSION_FULL], [PACKAGE_VERSION], [Alias for VERSION and PACKAGE_VERSION for meson compatibility])
COPYRIGHT_YEAR=copyright_year()
AC_DEFINE_UNQUOTED([COPYRIGHT_YEAR], ["$COPYRIGHT_YEAR"], [Copyright year])
AC_SUBST([COPYRIGHT_YEAR])

AC_CONFIG_MACRO_DIRS([m4])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 tar-ustar no-dist-gzip foreign subdir-objects])
AC_CONFIG_HEADERS([config.h])

if test x${enable_gtk_doc} = x"yes"; then
    AM_MAINTAINER_MODE([enable])
else
    if test x${enable_debug} != x"no"; then
        AM_MAINTAINER_MODE([enable])
    else
        AM_MAINTAINER_MODE()
    fi
fi

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_USE_SYSTEM_EXTENSIONS()

# ===================================================== #
#               Basic compiler settings                 #
# ===================================================== #
AC_PROG_CC()
AC_PROG_CXX()
AM_PROG_CC_C_O()
LT_PATH_LD()
AC_PROG_INSTALL()

# ===================================================== #
#                 Initialize libtool                    #
# ===================================================== #
LT_PREREQ([2.2.6])
LT_INIT([disable-static])
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])

# ===================================================== #
#                 Version information                   #
# ===================================================== #
PAROLE_VERINFO=parole_verinfo()
PAROLE_VERSION_API=parole_version_api()
PAROLE_VERSION_MAJOR=xdt_version_major()
PAROLE_VERSION_MINOR=xdt_version_minor()
PAROLE_VERSION_MICRO=xdt_version_micro()
AC_SUBST([PAROLE_VERINFO])
AC_SUBST([PAROLE_VERSION_API])
AC_SUBST([PAROLE_VERSION_MAJOR])
AC_SUBST([PAROLE_VERSION_MINOR])
AC_SUBST([PAROLE_VERSION_MICRO])

dnl define a short version string for parole
m4_append([parole_version_short], ["])
m4_append([parole_version_short], [xdt_version_major])
m4_append([parole_version_short], [.])
m4_append([parole_version_short], [xdt_version_minor])
m4_append([parole_version_short], ["])

dnl define parole's short version (major.minor)
AC_DEFINE([VERSION_SHORT], [parole_version_short], [Short version string of parole])

# ===================================================== #
#   Check for headers needed for standard interfaces    #
# ===================================================== #
AC_CHECK_HEADERS([errno.h signal.h stddef.h sys/ioctl.h fcntl.h\
                  string.h sys/stat.h sys/types.h sys/wait.h time.h \
                  unistd.h])

# ===================================================== #
#               Check for i18n support                  #
# ===================================================== #
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

# ===================================================== #
#             Check for required packages               #
# ===================================================== #
m4_define([gtk_minimum_version], [3.22.0])
m4_define([glib_minimum_version], [2.38.0])

m4_define([dbus_minimum_version], [0.60])
m4_define([dbus_glib_minimum_version], [0.70])

m4_define([libxfce4ui_minimum_version],[4.11.0])
m4_define([libxfce4util_minimum_version],[4.10.0])
m4_define([libxfconf_minimum_version], [4.10.0])
m4_define([libnotify_minimum_version], [0.7.8])
m4_define([libx11_minimum_version], [1.6.7])


XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GOBJECT], [gobject-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GMODULE], [gmodule-2.0], [glib_minimum_version])
XDT_CHECK_PACKAGE([GIO], [gio-2.0], [glib_minimum_version])

XDT_CHECK_PACKAGE([DBUS], [dbus-1], [dbus_minimum_version])
XDT_CHECK_PACKAGE([DBUS_GLIB], [dbus-glib-1], [dbus_glib_minimum_version])

XDT_CHECK_PACKAGE([LIBXFCE4UTIL], [libxfce4util-1.0],[libxfce4util_minimum_version])
XDT_CHECK_PACKAGE([XFCONF], [libxfconf-0], [libxfconf_minimum_version])

XDT_CHECK_PACKAGE_BINARY([GLIB_COMPILE_RESOURCES], [gio-2.0], [glib_compile_resources], [glib-compile-resources])

dnl ************************************
dnl *** Compile against GTK3 ***
dnl ************************************
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [libxfce4ui_minimum_version])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [gtk_minimum_version])

dnl ************************************
dnl *** GStreamer 1.0 ***
dnl ************************************
gstreamer_api_version=1.0
gstreamer_minimum_version=1.0.0
GST_API_VERSION=1.0
GST_REQS=1.0.0
GSTPLUG_REQS=1.0.0
XDT_CHECK_PACKAGE([GST], [gstreamer-${gstreamer_api_version}], [${gstreamer_minimum_version}])
XDT_CHECK_PACKAGE([GST_BASE], [gstreamer-base-${gstreamer_api_version}], [${gstreamer_minimum_version}])
XDT_CHECK_PACKAGE([GST_VIDEO], [gstreamer-video-${gstreamer_api_version}], [${gstreamer_minimum_version}])
XDT_CHECK_PACKAGE([GST_AUDIO], [gstreamer-audio-${gstreamer_api_version}], [${gstreamer_minimum_version}])
XDT_CHECK_PACKAGE([GST_PBUTILS], [gstreamer-pbutils-${gstreamer_api_version}], [${gstreamer_minimum_version}])
XDT_CHECK_PACKAGE([GST_TAG], [gstreamer-tag-${gstreamer_api_version}], [${gstreamer_minimum_version}])
GST_LIBS="$GST_LIBS -lgstbase-$GST_API_VERSION -lgstvideo-$GST_API_VERSION -lgstaudio-$GST_API_VERSION -lgstpbutils-$GST_API_VERSION -lgsttag-$GST_API_VERSION"

XDT_CHECK_OPTIONAL_FEATURE([X11],
                           [x11],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_X11], [gdk-x11-3.0], [gtk_minimum_version])
                             XDT_FEATURE_DEPENDENCY([LIBX11], [x11], [libx11_minimum_version])
                           ],
                           [the X11 windowing system])
XDT_CHECK_OPTIONAL_FEATURE([WAYLAND],
                           [wayland],
                           [
                             XDT_FEATURE_DEPENDENCY([GDK_WAYLAND], [gdk-wayland-3.0], [gtk_minimum_version])
                           ],
                           [the Wayland windowing system])
if test x"$ENABLE_X11" != x"yes" -a x"$ENABLE_WAYLAND" != x"yes"; then
  AC_MSG_ERROR([Either both X11 and Wayland support was disabled, or required dependencies are missing. One of the two must be enabled.])
fi

# ===================================================== #
#                  Check XF86keysysm                    #
# ===================================================== #
# Check for the AudioPlay sym other should exist
# if this one exists.
if test x"$ENABLE_X11" = x"yes"; then
  AC_CHECK_DECL(XF86XK_AudioPlay,have_XF86XK_AudioPlay=1,have_XF86XK_AudioPlay=0,[#include <X11/XF86keysym.h>])
  if test "x${ac_cv_have_decl_XF86XK_AudioPlay}" != "xno"; then
    AC_DEFINE(HAVE_XF86_KEYSYM, 1, [Define if XF86keysysm.h is available])
  fi
fi

# ===================================================== #
#                   TagLib support                      #
# ===================================================== #
TAGLIB_FOUND="no"
XDT_CHECK_OPTIONAL_PACKAGE([TAGLIB],
    [taglib], [1.4],
    [taglib],
    [tag lib], [yes])

if test x"$TAGLIB_FOUND" = x"yes"; then
TAGLIBC_LIBS=""
TAGLIBC_CFLAGS=""
AC_CHECK_LIB([tag_c], [taglib_file_new],
[
    AC_CHECK_HEADER([taglib/tag_c.h],
    [
    TAGLIBC_LIBS="-ltag_c"
    AC_DEFINE([HAVE_TAGLIBC], [1], [Define to 1 if tag_c is found])
    ])
], [-lm])
AC_SUBST([TAGLIBC_CFLAGS])
AC_SUBST([TAGLIBC_LIBS])
fi

# ===================================================== #
#               Check plugins to build                  #
# ===================================================== #

# Notify plugin.
#--------------------------
AC_ARG_ENABLE([notify-plugin], AS_HELP_STRING([--disable-notify-plugin], [Don't build parole notification plugin]),
  [ac_notify_plugin=$enableval], [ac_notify_plugin=yes])
AC_MSG_CHECKING([whether to build the notification plugin])
AM_CONDITIONAL([PAROLE_NOTIFY_PLUGIN], [test x"$ac_notify_plugin" = x"yes"])
AC_MSG_RESULT([$ac_notify_plugin])
# Depends on Libnotify - if not found cannot build
LIBNOTIFY_FOUND="no"
if test x"$ac_notify_plugin" = x"yes"; then
    XDT_CHECK_PACKAGE([LIBNOTIFY],
    [libnotify], [libnotify_minimum_version])
fi

# Tray plugin.
#--------------------------
AC_ARG_ENABLE([tray-plugin], AS_HELP_STRING([--disable-tray-plugin], [Don't build parole tray icon plugin]),
  [ac_tray_plugin=$enableval], [ac_tray_plugin=yes])
AC_MSG_CHECKING([whether to build the system tray plugin])
AM_CONDITIONAL([PAROLE_TRAY_PLUGIN], [test x"$ac_tray_plugin" = x"yes"])
AC_MSG_RESULT([$ac_tray_plugin])

# MPRIS2 plugin.
#--------------------------
AC_ARG_ENABLE([mpris2-plugin], AS_HELP_STRING([--disable-mpris2-plugin], [Don't build parole mpris2 plugin]),
  [ac_mpris2_plugin=$enableval], [ac_mpris2_plugin=yes])
AC_MSG_CHECKING([whether to build the mpris2 plugin])
AM_CONDITIONAL([PAROLE_MPRIS2_PLUGIN], [test x"$ac_mpris2_plugin" = x"yes"])
AC_MSG_RESULT([$ac_mpris2_plugin])

# ===================================================== #
#             Check for debugging support               #
# ===================================================== #
XDT_FEATURE_DEBUG([xdt_debug_default])

# ===================================================== #
#                  Check for Gtk doc                    #
# ===================================================== #
GTK_DOC_CHECK(1.9)

AC_CONFIG_FILES([
Makefile
data/Makefile
data/pixmaps/Makefile
data/interfaces/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/64x64/Makefile
data/icons/96x96/Makefile
data/icons/128x128/Makefile
data/icons/scalable/Makefile
data/mime/Makefile
data/desktop/Makefile
data/metainfo/Makefile
src/Makefile
src/misc/Makefile
src/misc/parole.h
src/common/Makefile
src/gst/Makefile
src/dbus/Makefile
src/plugins/Makefile
src/plugins/sample/Makefile
src/plugins/notify/Makefile
src/plugins/tray/Makefile
src/plugins/mpris2/Makefile
docs/Makefile
docs/plugin-api/version.xml
docs/plugin-api/Makefile
po/Makefile.in
])
AC_OUTPUT

echo "
------------------------------------------------------
            Parole Media Player $VERSION
        ====================================

        Installation:
        =============
        prefix:                         $prefix
        Debug:                          $enable_debug
        Building plugin api docs:       ${enable_gtk_doc}
        GStreamer API version:          ${gstreamer_api_version}
        GTK+ version:                   ${GTK_VERSION}

        Optional dependencies:
        ======================
        Taglib:                         ${TAGLIB_FOUND:-no}
        X11:                            ${ENABLE_X11:-no}
        Wayland:                        ${ENABLE_WAYLAND:-no}

        Plugins to build:
        =================
        Notify:                         ${ac_notify_plugin}
        System Tray icon:               ${ac_tray_plugin}
        MPRIS2:                         ${ac_mpris2_plugin}

------------------------------------------------------

Configuration finished, type make to compile"
