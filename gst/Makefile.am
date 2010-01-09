noinst_LTLIBRARIES = 			\
	libparolegst.la

INCLUDES =                              \
        -I$(top_srcdir)                 \
        -I$(top_srcdir)/common          \
	-I$(top_srcdir)/parole		\
        -DLOCALEDIR=\"$(localedir)\"    \
	-DPAROLE_COMPILATION            \
        -DG_LOG_DOMAIN=\"parole-gst\"

GENERATED_FILES =			\
	gstmarshal.c			\
	gstmarshal.h			\
	gst-enum-types.c		\
	gst-enum-types.h

libparolegst_la_SOURCES =		\
	$(GENERATED_FILES)		\
	parole-gst.c			\
	parole-gst.h			\
	parole-gst-iface.c		\
	parole-gst-iface.h
	
libparolegst_la_CFLAGS =		\
	$(GST_VIDEO_CFLAGS)             \
        $(GST_INTERFACES_CFLAGS)	\
	$(GTHREAD_CFLAGS)               \
	$(LIBXFCE4UI_CFLAGS)

libparolegst_la_LIBADD =		\
	$(top_builddir)/parole/libparole.la\
	$(top_builddir)/common/libparolecommon.la

parole_glib_enum_headers =		\
	parole-gst.h

if MAINTAINER_MODE

BUILT_SOURCES = 			\
	$(GENERATED_FILES)

gstmarshal.c: gstmarshal.list
	echo "#include \"gstmarshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=_gmarshal --body >> $@

gstmarshal.h: gstmarshal.list
	glib-genmarshal $< --prefix=_gmarshal --header > $@

gst-enum-types.h: stamp-enum-types.h
	@true
stamp-enum-types.h: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef __GST_ENUM_TYPE_H__\n#define __GST_ENUM_TYPE_H__\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GST_ENUM_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* __GST_ENUM_TYPE_H__ */" \
		$(parole_glib_enum_headers) ) > xgen-enum.h \
	&& (cmp -s xgen-enum.h gst-enum-types.h || cp xgen-enum.h gst-enum-types.h) \
	&& rm -f xgen-enum.h \
	&& echo timestamp > $(@F)

gst-enum-types.c: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include \"gst-enum-types.h\"\n\n" \
		--fhead "#include \"parole-gst.h\"\n\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(parole_glib_enum_headers) ) >> xgen-enum.c \
	&& cp xgen-enum.c gst-enum-types.c \
	&& rm -f xgen-enum.c

endif

EXTRA_DIST =					\
	gstmarshal.list

DISTCLEANFILES =				\
	$(BUILT_SOURCES)			\
	stamp-enum-types.h
