## Process this file with automake to produce Makefile.in

SUBDIRS = media-plugin

DEFINES = -DMOZILLA_STRICT_API -DXP_UNIX

# This is probably not the best thing, but the browser plugin 
# fails to compile with --enable-debug=full so overried CXXFLAGS
# to get rid of -Wundef -Wredundant-decls -Wmissing-declarations
CXXFLAGS = -Wall
if DEBUG_BROWSER_PLUGIN
CXXFLAGS += -Wextra -Wno-missing-field-initializers	\
	-Wno-unused-parameter 				\
	-Wmissing-noreturn -Wshadow 			\
	-Wpointer-arith -Wcast-align -Wformat-security 	\
	-Winit-self -Wmissing-include-dirs		\
	-Wmissing-format-attribute 		 	\
	-fstack-protector -O0 -g3 -Werror
endif

CFLAGS=$(CXXFLAGS)

INCLUDES =                                              \
        -fPIC                                           \
        -I$(top_builddir)                               \
        -I$(top_srcdir)                                 \
	-I$(top_srcdir)/parole                         	\
        -DPACKAGE_LOCALE_DIR=\"$(localedir)\"           \
        -DG_LOG_DOMAIN=\"parole-browser-plugin\"        \
        -DLIBEXECDIR="\"$(libexecdir)\""                \
	$(GECKO_CFLAGS)					\
        $(GTK_CFLAGS)                                   \
        $(DBUS_GLIB_CFLAGS)				\
	$(TAGLIBC_CFLAGS)				\
	$(LIBXFCE4UTIL_CFLAGS)				\
	$(DEFINES)

install_libexecdir = $(BROWSER_PLUGIN_DIR)

install_libexec_PROGRAMS = 				\
	parole-player.so

parole_player_so_SOURCES =     				\
	plugin.cpp					\
	plugin.h					\
	parole-plugin.c					\
	parole-plugin.h					\
	plugin_setup.cpp  				\
	plugin_setup.h					\
	plugin_types.cpp				\
	plugin_types.h					\
	np_entry.cpp    				\
	npn_gate.cpp					\
	npp_gate.cpp					\
	npupp.h

parole_player_so_LDFLAGS =                              \
	-Xcompiler					\
	-shared						\
        $(GTK_LIBS)                                     \
        $(DBUS_GLIB_LIBS)				\
	$(TAGLIBC_LIBS)					\
	$(LIBXFCE4UTIL_LIBS)				\
	$(top_builddir)/parole/libparole.la 	
