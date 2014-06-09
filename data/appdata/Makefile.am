#
# .appdata
#
appdata_in_files = parole.appdata.xml.in
appdatadir = $(datadir)/appdata
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST =					\
	parole.appdata.xml.in

CLEANFILES =					\
	$(appdata_in_files)			\
	$(appdata_DATA)