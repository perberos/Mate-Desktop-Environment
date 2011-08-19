plugindir = $(PLUGINDIR)/opensubtitles
uidir = $(plugindir)
plugin_PYTHON = opensubtitles.py hash.py

plugin_in_files = opensubtitles.totem-plugin.in

%.totem-plugin: %.totem-plugin.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

plugin_DATA = $(plugin_in_files:.totem-plugin.in=.totem-plugin)
ui_DATA = opensubtitles.ui

EXTRA_DIST = $(plugin_in_files) $(ui_DATA) opensubtitles.py hash.py

CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA) 
