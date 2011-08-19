## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = 1.6

# The name of the module
DOC_MODULE=MateCORBA2

# The top-level SGML file.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=$(top_srcdir)/src/orb/orb-core
EXTRA_HFILES=\
	$(top_srcdir)/include/matecorba/orb-core/allocators.h\
	$(top_srcdir)/include/matecorba/orb-core/matecorba-small.h

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="MATECORBA2_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=
CFILE_GLOB=

# Header files to ignore when scanning.
IGNORE_HFILES= corba-ops.h			\
	iop-profiles.h				\
	orb-core-export.h			\
	orb-core-private.h			\
	orbhttp.h				\
	$matecorba-debug.h				\
	matecorba-policy.h

# Images to copy into HTML directory.
HTML_IMAGES = 

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = 

# Other files to distribute.
extra_files =


# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS =
GTKDOC_LIBS = 


# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt


# include common portion ...
include $(top_srcdir)/gtk-doc.make

