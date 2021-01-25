bin_PROGRAMS = parole
SUBDIRS = 					\
	misc 					\
	common					\
	gst					\
	dbus					\
	plugins

AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src/misc			\
	-I$(top_srcdir)/data			\
	-I$(top_srcdir)/data/mime		\
	-I$(top_srcdir)/src/gst	                \
	-I$(top_srcdir)/src/dbus	                \
	-I$(top_srcdir)/src/common	                \
	-DLOCALEDIR=\"$(localedir)\"		\
	-DDESKTOPDIR=\"$(datadir)/applications\"\
	-DG_LOG_DOMAIN=\"parole\"		\
	-DPAROLE_PLUGINS_DIR=\"$(libdir)/parole-$(PAROLE_VERSION_API)\"\
	-DPAROLE_PLUGINS_DATA_DIR=\"$(datadir)/parole/parole-plugins-$(PAROLE_VERSION_API)\"\
	-DPAROLE_COMPILATION			\
	$(GTHREAD_CFLAGS)                       \
	$(GIO_CFLAGS)				\
	$(GMODULE_CFLAGS)			\
	$(DBUS_GLIB_CFLAGS)                     \
	$(LIBXFCE4UI_CFLAGS)                   \
	$(XFCONF_CFLAGS)                   \
	$(GST_VIDEO_CFLAGS)			\
	$(GST_INTERFACES_CFLAGS)		\
	$(TAGLIBC_CFLAGS)			\
	$(LIBX11_CFLAGS)

GENERATED_FILES =				\
	gmarshal.c				\
	gmarshal.h				\
	enum-gtypes.c				\
	enum-gtypes.h				\
	org.parole.media.list.h			\
	org.parole.media.player.h

parole_SOURCES =				\
	$(GENERATED_FILES)			\
	main.c					\
	parole-player.c				\
	parole-player.h				\
	parole-vis.c				\
	parole-vis.h				\
	parole-medialist.c			\
	parole-medialist.h			\
	parole-mediachooser.c			\
	parole-mediachooser.h			\
	parole-conf.c				\
	parole-conf.h				\
	parole-conf-dialog.c			\
	parole-conf-dialog.h			\
	parole-disc.c				\
	parole-disc.h				\
	parole-utils.c				\
	parole-utils.h				\
	parole-plugins-manager.c		\
	parole-plugins-manager.h		\
	parole-module.c				\
	parole-module.h				\
	parole-about.c				\
	parole-about.h				\
	parole-shortcuts.c				\
	parole-shortcuts.h				\
	parole-builder.c			\
	parole-builder.h			\
	parole-subtitle-encoding.c		\
	parole-subtitle-encoding.h		\
	parole-open-location.c			\
	parole-open-location.h			\
	parole-button.c				\
	parole-button.h				\
	parole-plugin-player.c			\
	parole-plugin-player.h

parole_LDADD =						   	\
	$(top_builddir)/src/misc/libparole.la 		   	\
	$(top_builddir)/src/dbus/libparoledbus.la 	   	\
	$(top_builddir)/src/gst/libparolegst.la 	   	\
	$(top_builddir)/src/common/libparolecommon.la      	\
	$(top_builddir)/src/common/libparolescreensaver.la 	\
	$(GTHREAD_LIBS)                         \
	$(GIO_LIBS)				\
	$(GMODULE_LIBS)				\
	$(DBUS_GLIB_LIBS)                       \
	$(LIBXFCE4UI_LIBS)                     \
	$(XFCONF_LIBS)                     \
	$(GST_VIDEO_LIBS)			\
	$(GST_INTERFACES_LIBS)			\
	$(TAGLIBC_LIBS)				\
	$(LIBX11_LIBS)

parole_glib_enum_headers =			\
	parole-conf.h				\
	parole-button.h

if PAROLE_CLUTTER_BACKEND
AM_CPPFLAGS +=			\
	$(CLUTTER_CFLAGS)				\
	$(CLUTTERGTK_CFLAGS)
parole_SOURCES +=				\
	parole-clutter.c			\
	parole-clutter.h
parole_LDADD +=				\
	$(CLUTTER_LIBS)				\
	$(CLUTTERGTK_LIBS)
endif

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
		--fhead "#ifndef SRC_ENUM_GTYPES_H_\n#define SRC_ENUM_GTYPES_H_\n#include <glib-object.h>\nG_BEGIN_DECLS\n" \
		--fprod "/* enumerations from \"@filename@\" */\n" \
		--vhead "GType @enum_name@_get_type (void);\n#define ENUM_GTYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
		--ftail "G_END_DECLS\n\n#endif /* SRC_ENUM_GTYPES_H_ */" \
		$(parole_glib_enum_headers) ) > xgen-enum.h \
	&& (cmp -s xgen-enum.h enum-gtypes.h || cp xgen-enum.h enum-gtypes.h) \
	&& rm -f xgen-enum.h \
	&& echo timestamp > $(@F)

enum-gtypes.c: $(parole_glib_enum_headers) Makefile
	( cd $(srcdir) && glib-mkenums \
		--fhead "#include \"enum-gtypes.h\"\n\n" \
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
	gmarshal.list               \
	org.parole.media.player.xml \
	org.parole.media.list.xml

DISTCLEANFILES =				\
	$(BUILT_SOURCES)			\
	stamp-enum-gtypes.h
