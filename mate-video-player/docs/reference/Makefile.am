## Process this file with automake to produce Makefile.in

# We require automake 1.6 at least.
AUTOMAKE_OPTIONS = 1.6

# This is a blank Makefile.am for using gtk-doc.
# Copy this to your project's API docs directory and modify the variables to
# suit your project. See the GTK+ Makefiles in gtk+/docs/reference for examples
# of using the various options.

# The name of the module, e.g. 'glib'.
DOC_MODULE=totem

# The top-level SGML file. You can change this if you want to.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting the functions and macros.
# e.g. DOC_SOURCE_DIR=../../../gtk
DOC_SOURCE_DIR=../../src

# Extra options to pass to gtkdoc-scangobj. Not normally needed.
SCANGOBJ_OPTIONS=

# Extra options to supply to gtkdoc-scan.
# e.g. SCAN_OPTIONS=--deprecated-guards="GTK_DISABLE_DEPRECATED" 
SCAN_OPTIONS=

# Extra options to supply to gtkdoc-mkdb.
# e.g. MKDB_OPTIONS=--sgml-mode --output-format=xml
MKDB_OPTIONS=--sgml-mode --output-format=xml --name-space=totem

# Extra options to supply to gtkdoc-mktmpl
# e.g. MKTMPL_OPTIONS=--only-section-tmpl
MKTMPL_OPTIONS=

# Extra options to supply to gtkdoc-fixref. Not normally needed.
# e.g. FIXXREF_OPTIONS=--extra-dir=../gdk-pixbuf/html --extra-dir=../gdk/html
FIXXREF_OPTIONS=

MKHTML_OPTIONS=--path="$(abs_builddir)"

# Used for dependencies. The docs will be rebuilt if any of these change.
# e.g. HFILE_GLOB=$(top_srcdir)/gtk/*.h
# e.g. CFILE_GLOB=$(top_srcdir)/gtk/*.c
HFILE_GLOB=$(top_srcdir)/src/*.h
CFILE_GLOB=$(top_srcdir)/src/*.c

# Header files to ignore when scanning.
# e.g. IGNORE_HFILES=gtkdebug.h gtkintl.h
IGNORE_HFILES = \
	totem-private.h			\
	ev-sidebar.h			\
	eggdesktopfile.h		\
	eggfileformatchooser.h		\
	egg-macros.h			\
	eggsmclient.h			\
	eggsmclient-private.h		\
	mate-screenshot-widget.h	\
	gsd-media-keys-window.h		\
	totemobject-marshal.h		\
	totemplaylist-marshal.h		\
	totemvideolist-marshal.h	\
	totem-profile.h			\
	totem-sample-vala-plugin.h	\
	totem-chapters-utils.h		\
	totem-cmml-parser.h		\
	totem-edit-chapter.h

# Files we don't want exported to plugins
IGNORE_HFILES += \
	bacon-video-widget-common.h	\
	baconvideowidget-marshal.h	\
	bacon-video-widget-gst-missing-plugins.h

# Files we don't want exported to plugins
IGNORE_HFILES += \
	bacon-video-widget-common.h			\
	baconvideowidget-marshal.h			\
	bacon-video-widget-gst-missing-plugins.h	\
	bacon-resize.h					\
	totem-playlist.h				\
	totem-fullscreen.h				\
	totem-module.h					\
	totem-time-label.h				\
	totem-plugin-manager.h				\
	totem-properties-view.h				\
	totem-open-location.h				\
	totem-python-module.h				\
	totem-statusbar.h				\
	totem-sidebar.h					\
	totem-session.h					\
	totem-resources.h				\
	totem-plugins-engine.h				\
	totem-options.h					\
	totem-preferences.h				\
	totem-dvb-setup.h				\
	gstscreenshot.h					\
	totem-uri.h					\
	totem-python-plugin.h				\
	totem-subtitle-encoding.h			\
	totem-menu.h					\
	video-utils.h					\
	debug.h						\
	totem-dnd-menu.h

# Plugin files
IGNORE_HFILES += \
	bacon-video-widget-properties.h	\
	totem-gallery.h			\
	totem-gallery-progress.h	\
	totem-screenshot.h		\
	totem-screenshot-plugin.h	\
	totem-skipto.h			\
	totem-skipto-plugin.h		\
	totem-time-entry.h		\
	totem-tracker-widget.h		\
	totem-marshal.h

# Images to copy into HTML directory.
# e.g. HTML_IMAGES=$(top_srcdir)/gtk/stock-icons/stock_about_24.png
HTML_IMAGES=

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
# e.g. content_files=running.sgml building.sgml changes-2.0.sgml
content_files = version.xml totem-plugins.xml

# SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
# These files must be listed here *and* in content_files
# e.g. expand_content_files=running.sgml
expand_content_files=

# CFLAGS and LDFLAGS for compiling gtkdoc-scangobj with your library.
# Only needed if you are using gtkdoc-scangobj to dynamically query widget
# signals and properties.
# e.g. INCLUDES=-I$(top_srcdir) -I$(top_builddir) $(GTK_DEBUG_FLAGS)
# e.g. GTKDOC_LIBS=$(top_builddir)/gtk/$(gtktargetlib)
GTKDOC_CFLAGS = \
	-I$(top_srcdir)/		\
	-I$(top_srcdir)/src		\
	-I$(top_srcdir)/src/backend	\
	-I$(top_srcdir)/src/plugins	\
	$(DEPENDENCY_CFLAGS)
GTKDOC_LIBS = \
	$(top_builddir)/src/libtotem_main.la	\
	$(DEPENDENCY_LIBS)

# This includes the standard gtk-doc make rules, copied by gtkdocize.
include $(top_srcdir)/gtk-doc.make

# Other files to distribute
# e.g. EXTRA_DIST += version.xml.in
EXTRA_DIST += version.xml.in

if ENABLE_GTK_DOC
TESTS_ENVIRONMENT = cd $(srcdir) && 
TESTS = $(GTKDOC_CHECK)
endif
