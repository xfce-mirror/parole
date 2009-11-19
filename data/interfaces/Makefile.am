interfaces =					\
	parole.ui				\
	parole-settings.ui			\
	playlist.ui				\
	mediachooser.ui				\
	save-playlist.ui			\
	plugins.ui

interfaces_h = 					\
	$(interfaces:.ui=_ui.h)

if MAINTAINER_MODE
BUILT_SOURCES = $(interfaces_h)

parole_ui.h: parole.ui
	exo-csource --static --strip-comments --strip-content --name=parole_ui $< > $@

parole-settings_ui.h: parole-settings.ui
	exo-csource --static --strip-comments --strip-content --name=parole_settings_ui $< > $@

playlist_ui.h: playlist.ui
	exo-csource --static --strip-comments --strip-content --name=playlist_ui $< > $@

mediachooser_ui.h: mediachooser.ui
	exo-csource --static --strip-comments --strip-content --name=mediachooser_ui $< > $@

save-playlist_ui.h: save-playlist.ui
	exo-csource --static --strip-comments --strip-content --name=save_playlist_ui $< > $@

plugins_ui.h: plugins.ui
	exo-csource --static --strip-comments --strip-content --name=plugins_ui $< > $@

endif

CLEANFILES =					\
	$(interfaces_h)

EXTRA_DIST =					\
	$(interfaces)				\
	$(interfaces_h)