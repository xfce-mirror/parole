INCLUDES =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-DG_LOG_DOMAIN=\"power-manager_plugin\" \
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	power-manager-plugin.la

power_manager_plugin_la_SOURCES =		\
	power-manager-plugin.c			\
	power-manager-provider.c		\
	power-manager-provider.h

power_manager_plugin_la_CFLAGS =		\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)			\
	$(DBUS_GLIB_CFLAGS)


power_manager_plugin_la_LDFLAGS =		\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)

#
# .desktop file
#
desktop_in_files = power-manager.desktop.in
desktopdir = $(datadir)/src/misc/parole-plugins-$(PAROLE_VERSION_API)
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 		  	\
	$(desktop_in_files)
    
DISTCLEANFILES = 		\
	$(desktop_DATA)