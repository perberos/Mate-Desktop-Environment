## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libmate

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="MATE_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libmate

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libmate/*.h
CFILE_GLOB=$(top_srcdir)/libmate/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=			\
	mate-mateconfP.h		\
	mate-i18nP.h		\
	mate-preferences.h	\
	mate-triggersP.h

INCLUDES = -I$(top_srcdir) -I$(top_builddir)
GTKDOC_LIBS =

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = version.xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Images to copy into HTML directory.
HTML_IMAGES =

include $(top_srcdir)/gtk-doc.make

EXTRA_DIST += version.xml.in
