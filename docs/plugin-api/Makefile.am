## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.8

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=Parole-Plugins

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../gtk
DOC_SOURCE_DIR=../../src/misc

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED"
SCAN_OPTIONS=--ignore-headers=config.h --rebuild-types

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-mkhtml
MKHTML_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/gtk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/gtk/*.c
CFILE_GLOB=$(top_srcdir)/src/misc/*.c
HFILE_GLOB=$(top_srcdir)/src/misc/*.h

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
#IGNORE_HFILES=

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files= 			\
	version.xml		\
	parole-plugins-tut.sgml

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. GTKDOC_CFLAGS=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
GTKDOC_CFLAGS=                  \
	$(GTK_CFLAGS)		\
        $(GLIB_CFLAGS)          \
        $(GOBJECT_CFLAGS)       \
	$(TAGLIBC_CFLAGS)	\
        -I$(top_builddir)/src/misc

GTKDOC_LIBS=                    \
	$(GTK_LIBS)		\
        $(GLIB_LIBS)            \
        $(GOBJECT_LIBS)		\
	$(TAGLIBC_LIBS)		\
	$(top_builddir)/src/misc/libparole.la

if ENABLE_GTK_DOC
include $(top_srcdir)/gtk-doc.make
doccheck: $(GTKDOC_CHECK)
	$(AM_V_GEN)ln -s $< $@

MOSTLYCLEANFILES = doccheck
TESTS = doccheck
else
EXTRA_DIST=
endif

EXTRA_DIST +=			\
	version.xml.in			\
	$(DOC_MAIN_SGML_FILE)	\
	parole-plugins-tut.sgml
