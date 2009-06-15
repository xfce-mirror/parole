bin_PROGRAMS = parole-media-player

GENERATED_FILES =				\
	gmarshal.c				\
	gmarshal.h				\
	enum-gtypes.c				\
	enum-gtypes.h

MOD_FLAGS = 					\
	-export_dynamic 			\
	-avoid-version 				\
	-module 				\
	-no-undefined

parole_media_player_SOURCES =			\
	main.c					\
	$(GENERATED_FILES)			\
	sidebar.c				\
	sidebar.h				\
	player.c				\
	player.h				\
	stream.c				\
	stream.h				\
	gst.c					\
	gst.h					\
	vis.c					\
	vis.h					\
	statusbar.c				\
	statusbar.h				\
	builder.c				\
	builder.h				\
	medialist.c				\
	medialist.h				\
	mediafile.c				\
	mediafile.h				\
	mediachooser.c				\
	mediachooser.h				\
	filters.c				\
	filters.h				\
	screensaver.c				\
	screensaver.h				\
	conf.c					\
	conf.h					\
	conf-dialog.c				\
	conf-dialog.h				\
	rc-utils.c				\
	rc-utils.h				\
	utils.c					\
	utils.h					\
	debug.c					\
	debug.h

parole_media_player_CFLAGS =			\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/data/mime		\
	-DLOCALEDIR=\"$(localedir)\"		\
	-DG_LOG_DOMAIN=\"parole-media-player\"	\
	-export-dynamic				\
	$(GTK_CFLAGS)                           \
        $(GLIB_CFLAGS)                          \
        $(GOBJECT_CFLAGS)                       \
        $(GTHREAD_CFLAGS)                       \
	$(GIO_CFLAGS)				\
        $(DBUS_CFLAGS)                          \
        $(DBUS_GLIB_CFLAGS)                     \
        $(LIBXFCE4GUI_CFLAGS)                   \
        $(LIBXFCE4UTIL_CFLAGS)			\
	$(GST_CFLAGS)				\
	$(GST_BASE_CFLAGS)			\
	$(GST_INTERFACES_CFLAGS)

parole_media_player_LDADD =			\
	$(GTK_LIBS)                             \
        $(GLIB_LIBS)                            \
        $(GOBJECT_LIBS)                         \
        $(GTHREAD_LIBS)                         \
	$(GIO_LIBS)				\
        $(DBUS_LIBS)                            \
        $(DBUS_GLIB_LIBS)                       \
        $(LIBXFCE4GUI_LIBS)                     \
        $(LIBXFCE4UTIL_LIBS)			\
	$(GST_LIBS)				\
	$(GST_BASE_LIBS)			\
	$(GST_INTERFACES_LIBS)

parole_glib_header =				\
	$(srcdir)/enum-glib.h

if MAINTAINER_MODE

BUILT_SOURCES = 				\
	gmarshal.c				\
	gmarshal.h				\
	enum-gtypes.c				\
	enum-gtypes.h

gmarshal.c: gmarshal.list
	echo "#include \"gmarshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=_gmarshal --body >> $@

gmarshal.h: gmarshal.list
	glib-genmarshal $< --prefix=_gmarshal --header > $@

enum-gtypes.h: $(parole_glib_header)
	( cd $(srcdir) && glib-mkenums \
                --fhead "#ifndef _ENUM_GTYPES_H\n#define _ENUM_GTYPES_H\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
                --fprod "/* enumerations from \"@filename@\" */\n" \
                --vhead "GType @enum_name@_get_type (void);\n#define ENUM_GTYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
                --ftail "G_END_DECLS\n\n#endif /* _ENUM_GTYPES_H__ */" \
                $(parole_glib_header) ) >> xgen \
        && (cmp -s xgen enum-gtypes.h || cp xgen enum-gtypes.h ) \
        && rm -f xgen xgen~

enum-gtypes.c: enum-gtypes.h
	( cd $(srcdir) && glib-mkenums \
                --fhead "#include <enum-gtypes.h>\n" \
                --fhead "#include \"enum-glib.h\"\n\n" \
                --fprod "\n/* enumerations from \"@filename@\" */\n" \
                --vhead "GType\n@enum_name@_get_type (void)\n{\n\tstatic GType type = 0;\n\tif (type == 0) {\n\tstatic const G@Type@Value values[] = {"\
                --vprod "\t{ @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
                --vtail "\t{ 0, NULL, NULL }\n\t};\n\ttype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n\treturn type;\n}\n" \
                enum-glib.h ) > xgen \
        && cp xgen enum-gtypes.c  \
        && rm -f xgen xgen~

endif

EXTRA_DIST =					\
	gmarshal.list				\
	enum-glib.h

DISTCLEANFILES =				\
	$(BUILT_SOURCES)