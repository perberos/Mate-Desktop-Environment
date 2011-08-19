## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libmatecanvas

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libmatecanvas

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libmatecanvas/*.h
CFILE_GLOB=$(top_srcdir)/libmatecanvas/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=mate-canvas-shape-private.h mate-canvas-i18n.h \
  mate-canvas-marshal.h gailcanvas.h gailcanvasitem.h gailcanvaswidget.h \
  gailcanvasgroup.h gailcanvasgroupfactory.h gailcanvasitemfactory.h \
  gailcanvastextfactory.h gailcanvaswidgetfactory.h gailcanvastext.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
INCLUDES = -I$(top_srcdir) $(LIBMATECANVAS_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/libmatecanvas/libmatecanvas-2.la \
  $(LIBMATECANVAS_LIBS)

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

include $(top_srcdir)/gtk-doc.make
