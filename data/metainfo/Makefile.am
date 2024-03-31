#
# .appdata
#
appdatadir = $(datadir)/metainfo
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = parole.appdata.xml.in

parole.appdata.xml: parole.appdata.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =					\
	$(appdata_in_files)

CLEANFILES =					\
	$(appdata_DATA)

check-appdata:
	appstream-util validate-relax --nonet parole.appdata.xml