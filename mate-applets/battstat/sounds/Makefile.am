
soundlistdir = $(sysconfdir)/sound/events
soundlist_in_files = battstat_applet.soundlist.in
soundlist_DATA = $(soundlist_in_files:.soundlist.in=.soundlist)

@INTLTOOL_SOUNDLIST_RULE@

EXTRA_DIST = $(soundlist_DATA) $(soundlist_in_files)

CLEANFILES = $(soundlist_DATA)

-include $(top_srcdir)/git.mk
