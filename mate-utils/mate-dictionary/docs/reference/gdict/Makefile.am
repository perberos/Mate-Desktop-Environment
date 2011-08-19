## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=gdict

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../../libgdict

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS=--type-init-func="g_type_init()"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/mate-dictionary/libgdict/*.h
CFILE_GLOB=$(top_srcdir)/mate-dictionary/libgdict/*.c

# Header files to ignore when scanning.
IGNORE_HFILES =			\
	gdict-enum-types.h	\
	gdict-marshal.h		\
	gdict-context-private.h \
	gdict-debug.h		\
	gdict-private.h		\
	stamp-gdict-enum-types.h \
	stamp-gdict-marshal.h

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = version.xml

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS =					\
	$(CFLAGS)				\
	$(CPPFLAGS)				\
	$(LIBGDICT_CFLAGS)			\
	-I$(srcdir)/../../../libgdict    	\
	-I$(top_builddir)/mate-dictionary/libgdict

GTKDOC_LIBS =						\
	$(LDFLAGS)					\
	$(LIBGDICT_LIBS)				\
	../../../libgdict/libgdict-1.0.la

include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += version.xml.in

#dist-hook-local:
#	mkdir $(distdir)/TEXT;          \
#	for f in $(srcdir)/TEXT/* ; do  \
#	  test -f $$f && cp -p $$f $(distdir)/TEXT; \
#	done
