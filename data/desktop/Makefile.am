MIME_LIST=$(top_srcdir)/data/mime/mime-type-list.txt

#
# .desktop file
#
desktop_in_files = org.xfce.Parole.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

org.xfce.Parole.desktop.in: org.xfce.Parole.desktop.in.in desktop.sh $(MIME_LIST)
	head -n9 $(srcdir)/org.xfce.Parole.desktop.in.in > $@ &&\
	$(SHELL) $(srcdir)/desktop.sh $(MIME_LIST) >> $@

EXTRA_DIST =					\
	desktop.sh				\
	org.xfce.Parole.desktop.in.in

CLEANFILES =					\
	$(desktop_in_files)			\
	$(desktop_DATA)
