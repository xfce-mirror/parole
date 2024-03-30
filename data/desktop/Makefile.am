MIME_LIST=$(top_srcdir)/data/mime/mime-type-list.txt

#
# .desktop file
#
desktop_in_files = org.xfce.Parole.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)

org.xfce.Parole.desktop.in: org.xfce.Parole.desktop.in.in desktop.sh $(MIME_LIST)
	head -n9 $(srcdir)/org.xfce.Parole.desktop.in.in > $@ &&\
	$(SHELL) $(srcdir)/desktop.sh $(MIME_LIST) >> $@

org.xfce.Parole.desktop: org.xfce.Parole.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =					\
	desktop.sh				\
	org.xfce.Parole.desktop.in.in

CLEANFILES =					\
	$(desktop_in_files)			\
	$(desktop_DATA)
