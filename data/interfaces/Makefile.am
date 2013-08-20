interfaces =					\
	parole.ui				\
	parole-settings.ui			\
	playlist.ui				\
	mediachooser.ui				\
	save-playlist.ui			\
	plugins.ui				\
	open-location.ui
	
interfaces_h = 					\
	$(interfaces:.ui=_ui.h)

interfaces =					\
	parole-3.ui				\
	parole-settings-3.ui			\
	playlist-3.ui				\
	mediachooser-3.ui				\
	save-playlist-3.ui			\
	plugins-3.ui				\
	open-location-3.ui

if MAINTAINER_MODE
BUILT_SOURCES = $(interfaces_h)

parole_ui.h: parole-3.ui
	xdt-csource --static --strip-comments --strip-content --name=parole_ui $< > $@

parole-settings_ui.h: parole-settings-3.ui
	xdt-csource --static --strip-comments --strip-content --name=parole_settings_ui $< > $@

playlist_ui.h: playlist-3.ui
	xdt-csource --static --strip-comments --strip-content --name=playlist_ui $< > $@

mediachooser_ui.h: mediachooser-3.ui
	xdt-csource --static --strip-comments --strip-content --name=mediachooser_ui $< > $@

save-playlist_ui.h: save-playlist-3.ui
	xdt-csource --static --strip-comments --strip-content --name=save_playlist_ui $< > $@

open-location_ui.h: open-location-3.ui
	xdt-csource --static --strip-comments --strip-content --name=open_location_ui $< > $@

plugins_ui.h: plugins-3.ui
	xdt-csource --static --strip-comments --strip-content --name=plugins_ui $< > $@
	
endif

DISTCLEANFILES =					\
	$(interfaces_h)
	
EXTRA_DIST =					\
	$(interfaces)				\
	$(interfaces_h)