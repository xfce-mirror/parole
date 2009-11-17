libexec_PROGRAMS = parole-media-plugin

parole_media_plugin_SOURCES =			\
	main.c					\
	parole-gst.c				\
	parole-gst.h				\
	org.parole.media.plugin.h

parole_media_plugin_CFLAGS =			\
	-I$(top_srcdir)                         \
        -I$(top_srcdir)/data                    \
        -I$(top_srcdir)/data/mime               \
	-I$(top_srcdir)/common               	\
        -DLOCALEDIR=\"$(localedir)\"            \
        -DG_LOG_DOMAIN=\"parole-media-plugin\"  \
	$(GTHREAD_CFLAGS)                       \
	$(GTK_CFLAGS)				\
	$(GST_VIDEO_CFLAGS)                     \
        $(GST_INTERFACES_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)

parole_media_plugin_LDADD =			\
	$(top_builddir)/common/libparoledbus.la \
	$(GTHREAD_LIBS)                       	\
	$(GTK_LIBS)				\
	$(GST_VIDEO_LIBS)                     	\
        $(GST_INTERFACES_LIBS)			\
	$(DBUS_GLIB_LIBS)

if MAINTAINER_MODE

BUILT_SOURCES =                                 \
        org.parole.media.plugin.h

org.parole.media.plugin.h: $(srcdir)/org.parole.media.plugin.xml
	dbus-binding-tool --mode=glib-server --prefix=parole_gst $< >$@

endif

DISTCLEANFILES =				\
	$(BUILT_SOURCES)