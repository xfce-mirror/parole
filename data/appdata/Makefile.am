#
# .appdata
#
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = parole.appdata.xml.in

EXTRA_DIST =					\
	parole.appdata.xml.in

CLEANFILES =					\
	$(appdata_DATA)