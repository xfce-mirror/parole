noinst_LTLIBRARIES =                    	\
        libparole.la

AM_CPPFLAGS =                              	\
        -I$(top_srcdir)                 	\
	-I$(top_srcdir)/src                 	\
        -DLOCALEDIR=\"$(localedir)\"		\
	-DPAROLE_COMPILATION

INST_HFILES =					\
	parole.h				\
	parole-file.h				\
	parole-stream.h				\
	parole-provider-player.h		\
	parole-provider-plugin.h		\
	parole-filters.h			\
	parole-debug.h				\
	parole-pl-parser.h

libparole_la_SOURCES =				\
	$(GENERATED_FILES)			\
	$(INST_HFILES)				\
	parole-provider-plugin.c		\
	parole-provider-player.c		\
	parole-file.c				\
	parole-stream.c				\
	parole-pl-parser.c			\
	parole-pl-parser.h			\
	parole-filters.c			\
	parole-filters.h			\
	parole-debug.c				\
	parole-debug.h

libparole_la_CFLAGS =				\
	$(GIO_CFLAGS)                           \
	$(GTK_CFLAGS)				\
	$(TAGLIBC_CFLAGS)			\
	$(LIBXFCE4UTIL_CFLAGS)

libparole_la_LIBADD =				\
	$(GTK_LIBS)				\
	$(TAGLIBC_LIBS)

paroleincludedir =				\
	$(includedir)/parole

paroleinclude_HEADERS =				\
	$(INST_HFILES)

GENERATED_FILES =				\
	parole-marshal.c			\
	parole-marshal.h			\
	parole-enum-types.c			\
	parole-enum-types.h

if MAINTAINER_MODE

BUILT_SOURCES =                         	\
	$(GENERATED_FILES)

parole-marshal.c: parole-marshal.list
	echo "#include \"parole-marshal.h\"" > $@ && \
	glib-genmarshal $< --prefix=parole_marshal --body >> $@

parole-marshal.h: parole-marshal.list
	glib-genmarshal $< --prefix=parole_marshal --header > $@

parole_glib_enum_headers =		\
	parole-stream.h			\
	parole-provider-player.h	\
	parole-pl-parser.h

parole-enum-types.h: stamp-enum-types.h
	@true
stamp-enum-types.h: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#ifndef SRC_MISC_PAROLE_ENUM_TYPES_H_\n#define SRC_MISC_PAROLE_ENUM_TYPES_H_\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "#ifndef __GTK_DOC_IGNORE__\nGType @enum_name@_get_type (void);\n#define PAROLE_ENUM_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n#endif\n" \
		--ftail "G_END_DECLS\n\n#endif /* SRC_MISC_PAROLE_ENUM_TYPES_H_ */" \
		$(parole_glib_enum_headers) ) > xgen-enum.h \
	&& (cmp -s xgen-enum.h parole-enum-types.h || cp xgen-enum.h parole-enum-types.h) \
	&& rm -f xgen-enum.h \
	&& echo timestamp > $(@F)

parole-enum-types.c: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include \"parole-enum-types.h\"\n\n" \
		--fhead "#include \"parole-stream.h\"\n\n" \
		--fhead "#include \"parole-provider-player.h\"\n\n" \
		--fhead "#include \"parole-pl-parser.h\"\n\n" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
		--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		$(parole_glib_enum_headers) ) >> xgen-enum.c \
	&& cp xgen-enum.c parole-enum-types.c \
	&& rm -f xgen-enum.c

endif

EXTRA_DIST =					\
	parole-marshal.list			\
	parole.h.in

DISTCLEANFILES =				\
	$(BUILT_SOURCES)			\
	stamp-enum-types.h
