MIME_LIST=$(top_srcdir)/data/mime/mime-type-list.txt

#
# .desktop file
#
desktop_in_files = org.xfce.Parole.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA =  $(desktop_in_files:.desktop.in=.desktop)

org.xfce.Parole.desktop.in: org.xfce.Parole.desktop.in.in desktop.sh $(MIME_LIST)
	$(SHELL) $(srcdir)/desktop.sh $(srcdir)/org.xfce.Parole.desktop.in.in $(MIME_LIST) $@

org.xfce.Parole.desktop: org.xfce.Parole.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =					\
	desktop.sh				\
	meson.build				\
	org.xfce.Parole.desktop.in.in

CLEANFILES =					\
	$(desktop_in_files)			\
	$(desktop_DATA)
