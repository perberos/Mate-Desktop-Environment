SUBDIRS = $(PLUGINS)
DIST_SUBDIRS = $(ALL_PLUGINS) totem

noinst_LTLIBRARIES = libtotemmodule.la

common_defines = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATECONF_PREFIX=\""/apps/totem"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DBINDIR=\""$(bindir)"\"			\
	-DTOTEM_PLUGIN_DIR=\""$(libdir)/totem/plugins"\"\
	-DG_LOG_DOMAIN=\""Totem"\"			\
	$(DISABLE_DEPRECATED)

modules_flags = -export_dynamic -avoid-version -module

libtotemmodule_la_SOURCES =		\
	totem-module.c			\
	totem-module.h			\
	totem-plugin.c			\
	totem-plugin.h			\
	totem-plugins-engine.c		\
	totem-plugins-engine.h		\
	totem-plugin-manager.c		\
	totem-plugin-manager.h

libtotemmodule_la_CPPFLAGS = \
	$(common_defines)	\
	$(AM_CPPFLAGS)

libtotemmodule_la_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(AM_CFLAGS)		\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/lib 				\
	-I$(top_builddir)/lib 				\
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_srcdir)/src/plugins

libtotemmodule_la_LDFLAGS = \
	$(AM_LDFLAGS)			

if ENABLE_PYTHON

SUBDIRS += totem

libtotemmodule_la_LIBADD = \
	$(DEPENDENCY_LIBS)	\
	$(PYTHON_LIBS)		\
	$(PYTHON_EXTRA_LIBS)	\
	$(PYGTK_LIBS)

libtotemmodule_la_SOURCES += \
	totem-python-module.c		\
	totem-python-module.h		\
	totem-python-plugin.c		\
	totem-python-plugin.h

libtotemmodule_la_CFLAGS += \
	$(NO_STRICT_ALIASING_CFLAGS)	\
	$(PYGTK_CFLAGS)			\
	$(PYTHON_CFLAGS)
endif
