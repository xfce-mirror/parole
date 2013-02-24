MIME_LIST=$(top_srcdir)/data/mime/mime-type-list.txt

#
# .desktop file
#
desktop_in_files = parole.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

parole.desktop.in: parole.desktop.in.in desktop.sh $(MIME_LIST)
	head -n9 $(srcdir)/parole.desktop.in.in > $@ &&\
	$(SHELL) $(srcdir)/desktop.sh $(MIME_LIST) >> $@

EXTRA_DIST =					\
	desktop.sh				\
	parole.desktop.in.in

CLEANFILES =					\
	$(desktop_in_files)			\
	$(desktop_DATA)