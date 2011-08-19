INCLUDES = -I$(top_srcdir) -I$(top_builddir)

## Process this file with automake to produce Makefile.in
AUTOMAKE_OPTIONS = 1.6

SUBDIRS = figures

# The name of the module, e.g. 'glib'.
DOC_MODULE=libmatecomponentui

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../matecomponent

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="MATECOMPONENT_UI_DISABLE_DEPRECATED"

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--sgml-mode --output-format=xml

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

# Used for dependencies.
HFILE_GLOB=
CFILE_GLOB=

# Header files to ignore when scanning.
IGNORE_HFILES= matecomponent-client-site.h		\
	matecomponent-control-internal.h		\
	matecomponent-dock-band.h			\
	matecomponent-dock-item-grip.h			\
	matecomponent-dock-item.h			\
	matecomponent-dock-layout.h			\
	matecomponent-dock.h				\
	matecomponent-embeddable-factory.h		\
	matecomponent-item-container.h			\
	matecomponent-item-handler.h			\
	matecomponent-transient.h			\
	matecomponent-ui-config-widget.h		\
	matecomponent-ui-engine-private.h		\
	matecomponent-ui-marshal.h			\
	matecomponent-ui-node-private.h		\
	matecomponent-ui-private.h			\
	matecomponent-ui-xml.h				\
	matecomponent-ui-sync-keys.h			\
	matecomponent-ui-sync-menu.h			\
	matecomponent-ui-sync-status.h			\
	matecomponent-ui-sync-toolbar.h		\
	matecomponent-ui-sync-status.h			\
	matecomponent-ui-toolbar-button-item.h		\
	matecomponent-ui-toolbar-control-item.h	\
	matecomponent-ui-toolbar-icon.h		\
	matecomponent-ui-toolbar-item.h		\
	matecomponent-ui-toolbar-popup-item.h		\
	matecomponent-ui-toolbar-separator-item.h	\
	matecomponent-ui-toolbar.h			\
	matecomponent-ui-toolbar-toggle-button-item.h	\
	matecomponent-canvas-item.h			\
	matecomponent-canvas-component.h		\
	matecomponent-ui-type-builtins.h


# Images to copy into HTML directory.
HTML_IMAGES = figures/control.png figures/control-frame.png

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files = ui-faq.xml

# Other files to distribute.
extra_files =


# CFLAGS and LDFLAGS for compiling scan program. Only needed
# if $(DOC_MODULE).types is non-empty.
GTKDOC_CFLAGS =					\
	@CFLAGS@				\
	-I$(top_srcdir)				\
	-DMATECOMPONENT_UI_INTERNAL			\
	$(MATECOMPONENTUI_CFLAGS)

GTKDOC_LIBS = $(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)


# If you need to override some of the declarations, place them in the
# $(DOC_MODULE)-overrides.txt file and uncomment the second line here.
DOC_OVERRIDES =
#DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt


# include common portion ...
include $(top_srcdir)/gtk-doc.make

