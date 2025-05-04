interfaces =					\
	parole.ui				\
	parole-settings.ui			\
	playlist.ui				\
	mediachooser.ui				\
	save-playlist.ui			\
	plugins.ui				\
	open-location.ui			\
	shortcuts.ui

BUILT_SOURCES =					\
	parole-resources.c

parole_resources_deps = $(shell $(GLIB_COMPILE_RESOURCES) --generate-dependencies --sourcedir $(srcdir) $(srcdir)/parole.gresource.xml)

parole-resources.c: $(srcdir)/parole.gresource.xml $(parole_resources_deps) Makefile
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $<

DISTCLEANFILES =				\
	parole-resources.c

EXTRA_DIST =					\
	$(interfaces)				\
	meson.build				\
	parole-resources.c			\
	parole.gresource.xml
