AM_CPPFLAGS =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-DG_LOG_DOMAIN=\"parole_notify\"    	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	parole-notify.la

parole_notify_la_SOURCES =				\
	notify-plugin.c				\
	notify-provider.c				\
	notify-provider.h

parole_notify_la_CFLAGS =				\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UI_CFLAGS)			\
	$(LIBXFCE4UTIL_CFLAGS)			\
	$(LIBNOTIFY_CFLAGS)

parole_notify_la_LDFLAGS =				\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)

parole_notify_la_LIBADD =				\
	$(top_builddir)/src/misc/libparole.la	\
	$(GTK_LIBS)				\
	$(LIBXFCE4UTIL_LIBS)			\
	$(LIBNOTIFY_LIBS)

#
# .desktop file
#
desktop_in_files = notify.desktop.in
desktopdir = $(datadir)/parole/parole-plugins-$(PAROLE_VERSION_API)
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 		  	\
	$(desktop_in_files)

DISTCLEANFILES = 		\
	$(desktop_DATA)
