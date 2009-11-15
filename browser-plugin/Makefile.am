SUBDIRS = 						\
	media-plugin

INCLUDES = 						\
	-fPIC						\
	-I$(top_builddir)				\
	-I$(top_srcdir) 				\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"		\
        -DG_LOG_DOMAIN=\"parole-browser-plugin\"	\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	$(GTK_CFLAGS)

plugindir = $(BROWSER_PLUGIN_DIR)

plugin_LTLIBRARIES = 	 				\
        parole-player.la

PLUGIN_FILES =						\
	npupp.h						\
	npapi.h						\
	npruntime.h

parole_player_la_SOURCES =				\
	$(PLUGIN_FILES)					\
	plugin.cpp					\
	parole-plugin.cpp				\
	parole-plugin.h


parole_player_la_CPPFLAGS =				\
	-DXP_UNIX 					\
	-DMOZ_X11
	-I$(top_srcdir)/browser-plugin

parole_player_la_LIBADD =				\
	$(GTK_LIBS)

parole_player_la_LDFLAGS =				\
	-avoid-version  				\
	-export-symbols $(srcdir)/plugin.symbols 	\
	-module

EXTRA_DIST = 						\
	$(PLUGIN_FILES) 
	plugin.symbols