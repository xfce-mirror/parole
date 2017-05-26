interfaces =					\
	parole.ui				\
	parole-settings.ui			\
	playlist.ui				\
	mediachooser.ui				\
	save-playlist.ui			\
	plugins.ui				\
	open-location.ui			\
	shortcuts.ui

interfaces_h = 					\
	$(interfaces:.ui=_ui.h)

if MAINTAINER_MODE
BUILT_SOURCES = $(interfaces_h)

parole_ui.h: parole.ui
	xdt-csource --static --strip-comments --strip-content --name=parole_ui $< > $@

parole-settings_ui.h: parole-settings.ui
	xdt-csource --static --strip-comments --strip-content --name=parole_settings_ui $< > $@

playlist_ui.h: playlist.ui
	xdt-csource --static --strip-comments --strip-content --name=playlist_ui $< > $@

mediachooser_ui.h: mediachooser.ui
	xdt-csource --static --strip-comments --strip-content --name=mediachooser_ui $< > $@

save-playlist_ui.h: save-playlist.ui
	xdt-csource --static --strip-comments --strip-content --name=save_playlist_ui $< > $@

open-location_ui.h: open-location.ui
	xdt-csource --static --strip-comments --strip-content --name=open_location_ui $< > $@

plugins_ui.h: plugins.ui
	xdt-csource --static --strip-comments --strip-content --name=plugins_ui $< > $@

shortcuts_ui.h: shortcuts.ui
	xdt-csource --static --strip-comments --strip-content --name=shortcuts_ui $< > $@

endif

DISTCLEANFILES =					\
	$(interfaces_h)

EXTRA_DIST =					\
	$(interfaces)				\
	$(interfaces_h)