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
	window-title.c

window_title_la_CFLAGS =			\
	$(PLATFORM_CFLAGS)			\
	$(GTK_CFLAGS)

window_title_la_LDFLAGS =			\
	-avoid-version				\
	-export-dynamic				\
	-module					\
	$(PLATFORM_LDFLAGS)
