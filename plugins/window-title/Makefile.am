INCLUDES =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-DG_LOG_DOMAIN=\"window_title\"    	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	window-title.la

window_title_la_SOURCES =			\
	window-title-plugin.c			\
	window-title-provider.c			\
	window-title-provider.h

window_title_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)

window_title_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)

#
# .desktop file
#
desktop_in_files = window-title.desktop.in
desktopdir = $(datadir)/parole/parole-plugins-$(PAROLE_VERSION_API)
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 		  	\
	$(desktop_in_files)
    
DISTCLEANFILES = 		\
	$(desktop_DATA)