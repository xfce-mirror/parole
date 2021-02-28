AM_CPPFLAGS =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-DG_LOG_DOMAIN=\"mpris2_plugin\"    	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	parole-mpris2.la

parole_mpris2_la_SOURCES =			\
	mpris2-plugin.c				\
	mpris2-provider.c			\
	mpris2-provider.h

parole_mpris2_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
    $(GST_BASE_CFLAGS)	\
	$(LIBXFCE4UTIL_CFLAGS)

parole_mpris2_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)

parole_mpris2_la_LIBADD =				\
	$(top_builddir)/src/misc/libparole.la	\
	$(GTK_LIBS)				\
	$(GST_LIBS)             \
	$(LIBXFCE4UTIL_LIBS)

#
# .desktop file
#
desktop_in_files = mpris2.desktop.in
desktopdir = $(datadir)/parole/parole-plugins-$(PAROLE_VERSION_API)
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 		  	\
	$(desktop_in_files)

DISTCLEANFILES = 		\
	$(desktop_DATA)
