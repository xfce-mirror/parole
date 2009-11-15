libexec_PROGRAMS = parole-media-plugin

parole_media_plugin_SOURCES =			\
	main.c					\
	parole-gst.c				\
	parole-gst.h

parole_media_plugin_CFLAGS =			\
	-I$(top_srcdir)                         \
        -I$(top_srcdir)/data                    \
        -I$(top_srcdir)/data/mime               \
        -DLOCALEDIR=\"$(localedir)\"            \
        -DG_LOG_DOMAIN=\"parole-media-plugin\"  \
	$(GTHREAD_CFLAGS)                       \
	$(GTK_CFLAGS)				\
	$(GST_VIDEO_CFLAGS)                     \
        $(GST_INTERFACES_CFLAGS)

parole_media_plugin_LDADD =			\
	$(GTHREAD_LIBS)                       	\
	$(GTK_LIBS)				\
	$(GST_VIDEO_LIBS)                     	\
        $(GST_INTERFACES_LIBS)