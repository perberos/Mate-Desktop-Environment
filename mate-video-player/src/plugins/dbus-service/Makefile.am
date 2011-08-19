plugindir = $(PLUGINDIR)/dbus
uidir = $(plugindir)
plugin_PYTHON = dbus-service.py

plugin_in_files = dbus-service.totem-plugin.in

%.totem-plugin: %.totem-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.totem-plugin.in=.totem-plugin)

EXTRA_DIST = $(plugin_in_files) dbus-service.py

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)
