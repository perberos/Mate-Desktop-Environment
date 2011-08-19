if ENABLE_JPEG
jpeg_LIB = $(top_builddir)/jpegutils/libeog-jpegutils.la
endif

toolbar_LIB = $(top_builddir)/cut-n-paste/toolbar-editor/libtoolbareditor.la

screensaver_LIB = $(top_builddir)/cut-n-paste/totem-screensaver/libtotemscrsaver.la

noinst_LTLIBRARIES = libeog.la

bin_PROGRAMS = eog

headerdir = $(prefix)/include/eog-@EOG_API_VERSION@/eog
header_DATA = $(INST_H_FILES)

MARSHAL_OUTPUT = 	\
	eog-marshal.h	\
	eog-marshal.c

NOINST_H_FILES =			\
	eog-session.h			\
	eog-util.h			\
	eog-pixbuf-util.h		\
	eog-preferences-dialog.h	\
	eog-config-keys.h		\
	eog-image-jpeg.h		\
	eog-image-private.h		\
	eog-uri-converter.h		\
	eog-metadata-reader.h		\
	eog-metadata-reader-jpg.h	\
	eog-metadata-reader-png.h	\
	eog-save-as-dialog-helper.h	\
	eog-print-image-setup.h         \
	eog-print-preview.h             \
	eog-print.h			\
	eog-module.h             	\
	eog-plugin-manager.h            \
	eog-plugin-engine.h             \
	uta.h				\
	eog-close-confirmation-dialog.h	\
	zoom.h

if ENABLE_PYTHON
NOINST_H_FILES += 			\
	eog-python-module.h		\
	eog-python-plugin.h
endif

INST_H_FILES =				\
	eog-application.h		\
	eog-debug.h			\
	eog-window.h			\
	eog-sidebar.h			\
	eog-dialog.h			\
	eog-properties-dialog.h		\
	eog-error-message-area.h	\
	eog-file-chooser.h		\
	eog-statusbar.h			\
	eog-thumb-nav.h			\
	eog-transform.h			\
	eog-image.h			\
	eog-enums.h			\
	eog-image-save-info.h		\
	eog-scroll-view.h		\
	eog-thumb-view.h		\
	eog-list-store.h		\
	eog-thumbnail.h			\
	eog-job-queue.h			\
	eog-jobs.h			\
	eog-plugin.h

libeog_la_SOURCES =			\
	eog-application.c		\
	eog-session.c			\
	eog-debug.c			\
	eog-util.c			\
	eog-pixbuf-util.c		\
	eog-window.c			\
	eog-sidebar.c			\
	eog-dialog.c			\
	eog-preferences-dialog.c	\
	eog-properties-dialog.c		\
	eog-error-message-area.c	\
	eog-file-chooser.c		\
	eog-statusbar.c			\
	eog-thumb-nav.c			\
	eog-transform.c			\
	eog-image.c			\
	eog-image-jpeg.c		\
	eog-image-save-info.c		\
	eog-scroll-view.c		\
	eog-thumb-view.c		\
	eog-list-store.c		\
	eog-thumbnail.c			\
	eog-job-queue.c			\
	eog-jobs.c			\
	eog-uri-converter.c		\
	eog-metadata-reader.c		\
	eog-metadata-reader-jpg.c	\
	eog-metadata-reader-png.c	\
	eog-save-as-dialog-helper.c	\
	eog-print-image-setup.c         \
	eog-print-preview.c           	\
	eog-print.c			\
	eog-module.c            	\
	eog-close-confirmation-dialog.c \
	eog-plugin.c            	\
	eog-plugin-manager.c            \
	eog-plugin-engine.c            	\
	uta.c				\
	zoom.c				\
	$(BUILT_SOURCES)		\
	$(NOINST_H_FILES)		\
	$(INST_H_FILES)

if HAVE_EXIF
INST_H_FILES +=				\
	eog-exif-util.h			\
	eog-exif-details.h
libeog_la_SOURCES +=			\
	eog-exif-util.c			\
	eog-exif-details.c
endif

if ENABLE_PYTHON
libeog_la_SOURCES += 			\
	eog-python-module.c		\
	eog-python-module.h		\
	eog-python-plugin.c		\
	eog-python-plugin.h
endif

if HAVE_EXEMPI
# We need to make sure eog-exif-details.h
# is only listed once in INST_H_FILES
# or the build will break with automake-1.11
if !HAVE_EXIF
INST_H_FILES +=				\
	eog-exif-details.h
endif !HAVE_EXIF
libeog_la_SOURCES +=			\
	eog-exif-details.c
endif HAVE_EXEMPI

libeog_la_CFLAGS =						\
	-I$(top_srcdir)/jpegutils				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-I$(top_srcdir)/cut-n-paste/totem-screensaver		\
	$(EOG_CFLAGS)						\
	$(WARN_CFLAGS)						\
	-DG_LOG_DOMAIN=\"EOG\"					\
	-DEOG_PREFIX=\""${prefix}"\"				\
	-DEOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOG_LOCALE_DIR=\""$(datadir)/locale"\"		\
	-DEOG_PIXMAPS_DIR=\""$(datadir)/pixmaps/eog"\"		\
	-DEOG_PLUGIN_DIR=\""$(libdir)/eog/plugins"\"

libeog_la_LIBADD = \
	$(EOG_LIBS)

if HAVE_LCMS
libeog_la_CFLAGS += 	\
	$(X11_CFLAGS)

libeog_la_LIBADD += \
	$(X11_LIBS)
endif

if ENABLE_PYTHON
libeog_la_CFLAGS += 			\
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)		\
	$(AM_CFLAGS)

libeog_la_LIBADD += \
	$(top_builddir)/bindings/python/eog.la
endif

libeog_la_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

eog_SOURCES = main.c

eog_CFLAGS = 							\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor		\
	-I$(top_srcdir)/cut-n-paste/totem-screensaver		\
	$(EOG_CFLAGS)						\
	-DEOG_DATA_DIR=\""$(pkgdatadir)"\"			\
	-DEOG_LOCALE_DIR=\""$(datadir)/locale"\"

eog_LDADD = 				\
	libeog.la			\
	$(EOG_LIBS)			\
	$(LIBJPEG)			\
	$(toolbar_LIB)			\
	$(screensaver_LIB)		\
	$(jpeg_LIB)

eog_LDFLAGS = -export-dynamic -no-undefined -export-symbols-regex "^[[^_]].*"

BUILT_SOURCES = 			\
	eog-enum-types.c		\
	eog-enum-types.h		\
	$(MARSHAL_OUTPUT)

eog-enum-types.h: eog-enum-types.h.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eog-enum-types.h.template $(INST_H_FILES)) > $@

eog-enum-types.c: eog-enum-types.c.template $(INST_H_FILES) $(GLIB_MKENUMS)
	$(AM_V_GEN)(cd $(srcdir) && $(GLIB_MKENUMS) --template eog-enum-types.c.template $(INST_H_FILES)) > $@

eog-marshal.h: eog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --internal --prefix=eog_marshal > $@

eog-marshal.c: eog-marshal.list $(GLIB_GENMARSHAL)
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --body --header --prefix=eog_marshal > $@

EXTRA_DIST = 				\
	eog-enum-types.h.template	\
	eog-enum-types.c.template	\
	eog-marshal.list

if HAVE_DBUS

BUILT_SOURCES += eog-application-service.h

EXTRA_DIST += eog-application-service.xml

eog-application-service.h: eog-application-service.xml
	$(AM_V_GEN)dbus-binding-tool --prefix=eog_application --mode=glib-server --output=eog-application-service.h $<

endif

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
