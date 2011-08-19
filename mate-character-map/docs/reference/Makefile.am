NULL =

# The name of the module.
DOC_MODULE = gucharmap

# Uncomment for versioned docs and specify the version of the module, e.g. '2'.
DOC_MODULE_VERSION = $(GUCHARMAP_API_VERSION)

# The top-level SGML file.
DOC_MAIN_SGML_FILE = gucharmap-docs.sgml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =

# Extra options to pass to gtkdoc-scangobj
SCANGOBJ_OPTIONS = --type-init-func="g_type_init()" 

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR = ../../gucharmap

# Used for dependencies
HFILE_GLOB = $(top_srcdir)/gucharmap/*.h
CFILE_GLOB = $(top_srcdir)/gucharmap/*.c

# Header files to ignore when scanning
IGNORE_HFILES = \
	gucharmap-chartable-accessible.h \
	gucharmap-chartable-cell-accessible.h \
	gucharmap-font-codepoint-list.h \
	gucharmap-mini-fontsel.h \
	gucharmap-print-operation.h \
	gucharmap-private.h \
	gucharmap-search-dialog.h \
	gucharmap-settings.h \
	gucharmap-window.h \
	unicode-blocks.h \
	unicode-categories.h \
	unicode-names.h \
	unicode-nameslist.h \
	unicode-scripts.h \
	unicode-unihan.h \
	unicode-versions.h \
	$(NULL)

# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
INCLUDES = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(GTK_CFLAGS)

GTKDOC_LIBS = \
	$(top_builddir)/gucharmap/libgucharmap@GUCHARMAP_LIBRARY_SUFFIX_U@.la \
	$(GTK_LIBS) \
	$(INTL_LIBS)

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =					\
	$(NULL)

# 	version.xml				\
# 	running.sgml				\
# 	building.sgml				\
# 	changes-1.2.sgml			\
# 	changes-2.0.sgml			\
# 	compiling.sgml				\
# 	directfb.sgml				\
# 	glossary.xml				\
# 	migrating-checklist.sgml		\
# 	migrating-GtkAction.sgml		\
# 	migrating-GtkComboBox.sgml		\
# 	migrating-GtkFileChooser.sgml		\
# 	migrating-GtkIconView.sgml		\
# 	migrating-GtkAboutDialog.sgml		\
# 	migrating-GtkColorButton.sgml		\
# 	migrating-GtkAssistant.sgml		\
# 	migrating-GtkRecentChooser.sgml 	\
# 	migrating-GtkLinkButton.sgml		\
# 	migrating-GtkBuilder.sgml		\
#         migrating-GtkTooltip.sgml		\
# 	objects_grouped.sgml			\
# 	osx.sgml				\
# 	question_index.sgml			\
# 	resources.sgml				\
# 	text_widget.sgml			\
# 	tree_widget.sgml			\
# 	windows.sgml				\
# 	x11.sgml				\
# 	gtk-query-immodules-2.0.xml		\
# 	gtk-update-icon-cache.xml		\
# 	gtk-builder-convert.xml			\
# 	visual_index.xml

expand_content_files = 				\
	$(NULL)

# Images to copy into HTML directory
HTML_IMAGES = \
	$(NULL)

# Extra options to supply to gtkdoc-fixref
# FIXMEchpe
FIXXREF_OPTIONS = \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/gobject \
	--extra-dir=$(GLIB_PREFIX)/share/gtk-doc/html/glib \
	--extra-dir=$(ATK_PREFIX)/share/gtk-doc/html/atk \
	--extra-dir=$(PANGO_PREFIX)/share/gtk-doc/html/pango \
	--extra-dir=$(CAIRO_PREFIX)/share/gtk-doc/html/cairo

include $(top_srcdir)/gtk-doc.make

-include $(top_srcdir)/git.mk
