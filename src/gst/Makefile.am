noinst_LTLIBRARIES = 			\
	libparolegst.la

AM_CPPFLAGS =                           \
        -I$(top_srcdir)                 \
        -I$(top_srcdir)/src             \
        -I$(top_srcdir)/src/common      \
	-I$(top_srcdir)/src/misc	\
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
	parole-gst.h

libparolegst_la_CFLAGS =		\
	$(GST_VIDEO_CFLAGS)             \
        $(GST_INTERFACES_CFLAGS)	\
	$(GTHREAD_CFLAGS)               \
	$(LIBXFCE4UI_CFLAGS)            \
	$(PBUTILS_CFLAGS)

libparolegst_la_LIBADD =		\
	$(top_builddir)/src/misc/libparole.la\
	$(top_builddir)/src/common/libparolecommon.la\
	$(GST_LIBS)		\
	$(GST_PBUTILS_LIBS)

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
		--fhead "#ifndef SRC_GST_GST_ENUM_TYPES_H_\n#define SRC_GST_GST_ENUM_TYPES_H_\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define GST_ENUM_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* SRC_GST_GST_ENUM_TYPES_H_ */" \
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
