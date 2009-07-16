INCLUDES =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-DG_LOG_DOMAIN=\"stream_properties\"    \
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

plugins_LTLIBRARIES =				\
	stream-properties.la

stream_properties_la_SOURCES =			\
	stream-properties.c

stream_properties_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)

stream_properties_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)
