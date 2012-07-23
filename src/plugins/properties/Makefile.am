INCLUDES =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-DG_LOG_DOMAIN=\"stream_properties\"    \
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	stream-properties.la

stream_properties_la_SOURCES =			\
	stream-properties-plugin.c		\
	stream-properties-provider.c		\
	stream-properties-provider.h

stream_properties_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(TAGLIBC_CFLAGS)			\
	$(LIBXFCE4UTIL_CFLAGS)

stream_properties_la_LIBADD =			\
	$(top_builddir)/src/misc/libparole.la	\
	$(GMODULE_LIBS)				\
	$(GTK_LIBS)				\
	$(LIBXFCE4UTIL_LIBS)			\
	$(TAGLIBC_LIBS)

stream_properties_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)

#
# .desktop file
#
desktop_in_files = stream-properties.desktop.in
desktopdir = $(datadir)/src/misc/parole-plugins-$(PAROLE_VERSION_API)
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = 		  	\
	$(desktop_in_files)
    
DISTCLEANFILES = 		\
	$(desktop_DATA)
