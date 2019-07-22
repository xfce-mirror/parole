#
# .appdata
#
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/metainfo
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = parole.appdata.xml.in

EXTRA_DIST =					\
	$(appdata_in_files)

CLEANFILES =					\
	$(appdata_DATA)

check-appdata:
	appstream-util validate-relax --nonet parole.appdata.xml