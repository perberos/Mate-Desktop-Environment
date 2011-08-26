soundlistdir = $(sysconfdir)/sound/events

soundlist_in_files = \
	mate.soundlist.in		\
	gtk2-events.soundlist.in

soundlist_DATA = $(soundlist_in_files:.soundlist.in=.soundlist)

man_MANS = mate-options.7

metadata_in_files = mate-default.xml.in
metadatadir = $(datadir)/mate-background-properties
metadata_DATA = $(metadata_in_files:.xml.in=.xml)

mate-default.xml.in: mate-default.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/mate-default.xml.in.in > mate-default.xml.in.tmp \
	&& mv mate-default.xml.in.tmp mate-default.xml.in

@INTLTOOL_SOUNDLIST_RULE@
@INTLTOOL_XML_RULE@

noinst_DATA = mate-default.xml.in
CLEANFILES = $(soundlist_DATA) mate-default.xml mate-default.xml.in

EXTRA_DIST = $(soundlist_in_files) $(soundlist_DATA) $(man_MANS) mate-default.xml.in.in
