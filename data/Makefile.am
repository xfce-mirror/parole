SUBDIRS = 		\
	interfaces	\
	pixmaps		\
	icons		\
	mime

#
# .desktop file
#
desktop_in_files = parole-media-player.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =					\
	$(desktop_in_files)

DISTCLEANFILES =				\
	$(desktop_DATA)