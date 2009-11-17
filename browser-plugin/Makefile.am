## Process this file with automake to produce Makefile.in

SUBDIRS = media-plugin

DEFINES = -DMOZILLA_STRICT_API -DXP_UNIX

INCLUDES =                                              \
        -fPIC                                           \
        -I$(top_builddir)                               \
        -I$(top_srcdir)                                 \
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"           \
        -DG_LOG_DOMAIN=\"parole-browser-plugin\"        \
        -DLIBEXECDIR="\"$(libexecdir)\""                \
        $(GTK_CFLAGS)                                   \
        $(DBUS_GLIB_CFLAGS)				\
	$(GECKO_CFLAGS)					\
	$(DEFINES)

install_libexecdir = $(libdir)/mozilla/plugins

install_libexec_PROGRAMS = 				\
	parole-player.so

parole_player_so_SOURCES =     				\
	plugin.cpp					\
	plugin.h					\
	plugin_setup.cpp  				\
	plugin_setup.h					\
	plugin_types.cpp				\
	plugin_types.h					\
	np_entry.cpp    				\
	npupp.h						\
	npp_gate.cpp

parole_player_so_LDFLAGS =                              \
	-Xcompiler					\
	-shared						\
        $(GTK_LIBS)                                     \
        $(DBUS_GLIB_LIBS)

