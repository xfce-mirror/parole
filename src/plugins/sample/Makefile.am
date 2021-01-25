noinst_LTLIBRARIES =				\
	sample-plugin.la

AM_CPPFLAGS =					\
	-I$(top_builddir)			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	-DG_LOG_DOMAIN=\"sample_plugin\"    	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"

pluginsdir = 					\
	$(libdir)/parole-$(PAROLE_VERSION_API)

sample_plugin_la_SOURCES =			\
	sample-plugin.c				\
	sample-provider.c			\
	sample-provider.h

sample_plugin_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)

sample_plugin_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)
