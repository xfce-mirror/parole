bin_PROGRAMS = parole

noinst_LTLIBRARIES =                    	\
        libparole.la

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/data			\
	-I$(top_srcdir)/data/mime		\
	-I$(top_srcdir)/gst	                \
	-I$(top_srcdir)/dbus	                \
	-I$(top_srcdir)/common	                \
	-DLOCALEDIR=\"$(localedir)\"		\
	-DG_LOG_DOMAIN=\"parole\"		\
	-DPAROLE_PLUGINS_DIR=\"$(libdir)/parole-$(PAROLE_VERSION_API)\"\
        $(GTHREAD_CFLAGS)                       \
	$(GIO_CFLAGS)				\
        $(DBUS_GLIB_CFLAGS)                     \
        $(LIBXFCE4GUI_CFLAGS)                   \
	$(GST_VIDEO_CFLAGS)			\
	$(GST_INTERFACES_CFLAGS)		\
	$(TAGLIBC_CFLAGS)

PAROLE_LDFLAGS =				\
	-export-dynamic 			\
	-no-undefined				\
	-export-symbols-regex "^[[^_]].*"

PAROLE_LIBS = 					\
	$(top_builddir)/dbus/libparoledbus.la 	\
	$(top_builddir)/gst/libparolegst.la 	\
	$(top_builddir)/common/libparolecommon.la\
        $(GTHREAD_LIBS)                         \
	$(GIO_LIBS)				\
        $(DBUS_GLIB_LIBS)                       \
        $(LIBXFCE4GUI_LIBS)                     \
	$(GST_VIDEO_LIBS)			\
	$(GST_INTERFACES_LIBS)			\
	$(TAGLIBC_LIBS)

GENERATED_FILES =				\
	gmarshal.c				\
	gmarshal.h				\
	enum-gtypes.c				\
	enum-gtypes.h				\
	org.parole.media.list.h			\
	org.parole.media.player.h

INST_HFILES =					\
	parole.h				\
	parole-plugin.h				\
	parole-file.h

NOINST_HFILES =					\
	parole-player.h				\
	parole-vis.h				\
	parole-statusbar.h			\
	parole-mediachooser.h			\
	parole-filters.h			\
	parole-screensaver.h			\
	parole-conf.h				\
	parole-conf-dialog.h			\
	parole-rc-utils.h			\
	parole-utils.h				\
	parole-disc.h				\
	parole-medialist.h			\
	parole-debug.h				\
	parole-plugins-manager.h		\
	parole-module.h				\
	parole-pl-parser.h			\
	parole-about.h				\
	parole-builder.h			\
	parole-subtitle-encoding.h		\
	parole-open-location.h			\
	parole-disc-menu.h			\
	parole-session.h			\
	parole-button.h

libparole_headers =				\
	$(INST_HFILES)

libparole_includedir =				\
	$(includedir)/parole

libparole_include_HEADERS =			\
	$(libparole_headers)

libparole_la_SOURCES =				\
	$(GENERATED_FILES)			\
	$(INST_HFILES)				\
	$(NOINST_HFILES)			\
	parole-player.c				\
	parole-file.c				\
	parole-vis.c				\
	parole-statusbar.c			\
	parole-medialist.c			\
	parole-mediachooser.c			\
	parole-filters.c			\
	parole-screensaver.c			\
	parole-conf.c				\
	parole-conf-dialog.c			\
	parole-disc.c				\
	parole-rc-utils.c			\
	parole-utils.c				\
	parole-debug.c				\
	parole-plugins-manager.c		\
	parole-plugin.c				\
	parole-module.c				\
	parole-pl-parser.c			\
	parole-about.c				\
	parole-builder.c			\
	parole-subtitle-encoding.c		\
	parole-open-location.c			\
	parole-disc-menu.c			\
	parole-session.c			\
	parole-button.c

libparole_la_LDFLAGS =				\
	$(PAROLE_LDFLAGS)

libparole_la_LIBADD =				\
	$(PAROLE_LIBS)

parole_SOURCES =				\
	main.c

parole_LDADD =					\
	libparole.la

parole_LDFLAGS =				\
	$(PAROLE_LDFLAGS)

parole_glib_enum_headers =			\
	parole-plugin.h				\
	parole-pl-parser.h			\
	parole-conf.h				\
	parole-button.h

if MAINTAINER_MODE

BUILT_SOURCES = 				\
	$(GENERATED_FILES)

gmarshal.c: gmarshal.list
	echo "#include \"gmarshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=_gmarshal --body >> $@

gmarshal.h: gmarshal.list
	glib-genmarshal $< --prefix=_gmarshal --header > $@

enum-gtypes.h: stamp-enum-gtypes.h
	@true
stamp-enum-gtypes.h: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __ENUM_GTYPES_H__\n#define __ENUM_GTYPES_H__\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define ENUM_GTYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* __ENUM_GTYPES_H__ */" \
		$(parole_glib_enum_headers) ) > xgen-enum.h \
	&& (cmp -s xgen-enum.h enum-gtypes.h || cp xgen-enum.h enum-gtypes.h) \
	&& rm -f xgen-enum.h \
	&& echo timestamp > $(@F)

enum-gtypes.c: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include \"enum-gtypes.h\"\n\n" \
		--fhead "#include \"parole-gst.h\"\n\n" \
		--fhead "#include \"parole-plugin.h\"\n\n" \
		--fhead "#include \"parole-pl-parser.h\"\n\n" \
		--fhead "#include \"parole-stream.h\"\n\n" \
		--fhead "#include \"parole-conf.h\"\n\n" \
		--fhead "#include \"parole-button.h\"\n\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(parole_glib_enum_headers) ) >> xgen-enum.c \
	&& cp xgen-enum.c enum-gtypes.c \
	&& rm -f xgen-enum.c

org.parole.media.list.h: $(srcdir)/org.parole.media.list.xml
	dbus-binding-tool --mode=glib-server --prefix=parole_media_list $< >$@

org.parole.media.player.h: $(srcdir)/org.parole.media.player.xml
	dbus-binding-tool --mode=glib-server --prefix=parole_player $< >$@

endif

EXTRA_DIST =					\
	gmarshal.list

DISTCLEANFILES =				\
	$(BUILT_SOURCES)			\
	stamp-enum-gtypes.h
