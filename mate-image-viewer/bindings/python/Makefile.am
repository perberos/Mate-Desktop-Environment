noinst_LTLIBRARIES = \
	eog.la

nodist_eog_la_SOURCES = \
	eog.c

eog_la_LDFLAGS = \
	-module -avoid-version

eog_la_LIBADD = 		\
	$(PYTHON_LIB_LOC)       \
	$(PYTHON_LIBS)		\
	$(PYTHON_EXTRA_LIBS)	\
	$(PYGTK_LIBS)

eog_la_CFLAGS = 					\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	-I$(top_srcdir)/cut-n-paste/toolbar-editor	\
	$(EOG_CFLAGS)					\
	$(NO_STRICT_ALIASING_CFLAGS)			\
	$(PYGTK_CFLAGS)					\
	$(PYTHON_CFLAGS)				\
	$(AM_CFLAGS)

$(top_builddir)/src/eog-enum-types.h:
	$(AM_V_GEN)cd $(top_builddir)/src && $(MAKE) eog-enum-types.h

eog.c: eog.defs eog.override $(top_builddir)/src/eog-enum-types.h
	$(AM_V_GEN)( cd $(srcdir) && $(PYGOBJECT_CODEGEN) \
		--register $(PYGOBJECT_DEFSDIR)/gio-types.defs \
		--register $(PYGTK_DEFSDIR)/pango-types.defs \
		--register $(PYGTK_DEFSDIR)/gdk-types.defs \
		--register $(PYGTK_DEFSDIR)/gtk-types.defs \
		--override $*.override \
		--prefix py$* $(<F) ) > $@

BINDING_EOG_HEADERS_SRCDIR_IN = 			\
	src/eog-application.h				\
	src/eog-window.h				\
	src/eog-dialog.h				\
	src/eog-properties-dialog.h			\
	src/eog-message-area.h				\
	src/eog-statusbar.h				\
	src/eog-thumb-nav.h				\
	src/eog-image.h					\
	src/eog-scroll-view.h				\
	src/eog-thumb-view.h				\
	src/eog-list-store.h				\
	src/eog-job-queue.h				\
	src/eog-jobs.h					\
	src/eog-plugin.h

BINDING_HEADERS_BUILDDIR_IN =

BINDING_EOG_HEADERS_SRCDIR := $(addprefix $(top_srcdir)/,$(BINDING_EOG_HEADERS_SRCDIR_IN))

BINDING_HEADERS_BUILDDIR   := $(addprefix $(top_builddir)/,$(BINDING_HEADERS_BUILDDIR_IN))

regenerate-python-binding:
	$(PYGTK_H2DEF) $(sort $(BINDING_EOG_HEADERS_SRCDIR) $(BINDING_HEADERS_BUILDDIR)) > eog.defs.new

BUILT_SOURCES = \
	eog.c

EXTRA_DIST = 		\
	eog.override	\
	eog.defs

CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

-include $(top_srcdir)/git.mk
