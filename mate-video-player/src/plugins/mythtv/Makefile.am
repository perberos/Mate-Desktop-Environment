modules_flags = -export_dynamic -avoid-version -module

plugindir = $(PLUGINDIR)/mythtv
plugin_LTLIBRARIES = libtotem_mythtv.la

plugin_in_files = mythtv.totem-plugin.in

%.totem-plugin: %.totem-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

install-data-local: $(schema_DATA)
if MATECONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
		MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
	done \
	fi
endif

plugin_DATA = $(plugin_in_files:.totem-plugin.in=.totem-plugin)

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

libtotem_mythtv_la_SOURCES = totem-mythtv.c
libtotem_mythtv_la_LDFLAGS = $(modules_flags)
libtotem_mythtv_la_CPPFLAGS = $(common_defines)
libtotem_mythtv_la_LIBADD = $(GMYTH_LIBS)

libtotem_mythtv_la_CFLAGS =	\
	$(DEPENDENCY_CFLAGS)	\
	$(GMYTH_CFLAGS)		\
	$(WARN_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(AM_CFLAGS)		\
	-I$(top_srcdir)/	\
	-I$(top_srcdir)/src	\
	-I$(top_srcdir)/src/backend \
	-I$(top_srcdir)/src/plugins

EXTRA_DIST = $(plugin_in_files) $(schema_in_files)

CLEANFILES = $(noinst_DATA) $(BUILT_SOURCES) $(schema_DATA)
DISTCLEANFILES = $(plugin_DATA)

