modules_flags = -export_dynamic -avoid-version -module

plugindir = $(PLUGINDIR)/bemused
plugin_LTLIBRARIES = libbemused.la

plugin_in_files = bemused.totem-plugin.in

%.totem-plugin: %.totem-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

noinst_DATA = $(plugin_in_files:.totem-plugin.in=.totem-plugin)

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

libbemused_la_SOURCES = totem-bemused.c
libbemused_la_LDFLAGS = $(modules_flags)
libbemused_la_CPPFLAGS = $(common_defines)
libbemused_la_LIBADD = $(BEMUSED_LIBS)

libbemused_la_CFLAGS =			\
	$(DEPENDENCY_CFLAGS)		\
	$(WARN_CFLAGS)			\
	$(BEMUSED_CFLAGS)		\
	$(AM_CFLAGS)			\
	-I$(top_srcdir)/		\
	-I$(top_srcdir)/src		\
	-I$(top_srcdir)/src/plugins	\
	-I$(top_srcdir)/src/backend

EXTRA_DIST = $(plugin_in_files)

CLEANFILES = $(noinst_DATA) $(BUILT_SOURCES)
DISTCLEANFILES = $(plugin_DATA)

