libexec_PROGRAMS = parole-media-plugin

parole_media_plugin_SOURCES =			\
	main.c					\
	parole-plugin-player.c			\
	parole-plugin-player.h			\
	org.parole.media.plugin.h

parole_media_plugin_CFLAGS =			\
	-I$(top_srcdir)                         \
	-I$(top_srcdir)/parole                  \
        -I$(top_srcdir)/data                    \
        -I$(top_srcdir)/data/mime               \
	-I$(top_srcdir)/dbus               	\
	-I$(top_srcdir)/gst               	\
	-I$(top_srcdir)/common               	\
        -DLOCALEDIR=\"$(localedir)\"            \
        -DG_LOG_DOMAIN=\"parole-media-plugin\"  \
	-DPAROLE_COMPILATION            	\
	$(GTHREAD_CFLAGS)                       \
	$(GTK_CFLAGS)				\
	$(GST_VIDEO_CFLAGS)                     \
        $(GST_INTERFACES_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)			\
	$(LIBXFCE4GUI_CFLAGS)			\
	$(TAGLIBC_CFLAGS)

parole_media_plugin_LDADD =			\
	$(top_builddir)/dbus/libparoledbus.la 	\
	$(top_builddir)/gst/libparolegst.la 	\
	$(top_builddir)/common/libparolescreensaver.la\
	$(GTHREAD_LIBS)                       	\
	$(GTK_LIBS)				\
	$(GST_VIDEO_LIBS)                     	\
        $(GST_INTERFACES_LIBS)			\
	$(DBUS_GLIB_LIBS)			\
	$(LIBXFCE4GUI_LIBS)			\
	$(TAGLIBC_LIBS)

if MAINTAINER_MODE

BUILT_SOURCES =                                 \
        org.parole.media.plugin.h

org.parole.media.plugin.h: $(srcdir)/org.parole.media.plugin.xml
	dbus-binding-tool --mode=glib-server --prefix=parole_gst $< >$@

endif

DISTCLEANFILES =				\
	$(BUILT_SOURCES)