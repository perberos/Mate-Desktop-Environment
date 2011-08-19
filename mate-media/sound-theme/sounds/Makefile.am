NULL =

sounddir = $(datadir)/sounds/mate/default/alerts

sound_DATA =			\
	bark.ogg		\
	drip.ogg		\
	glass.ogg		\
	sonar.ogg		\
	$(NULL)

metadata_in_files = mate-sounds-default.xml.in
metadatadir = $(pkgdatadir)/sounds
metadata_DATA = $(metadata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

noinst_DATA = mate-sounds-default.xml.in
CLEANFILES = mate-sounds-default.xml mate-sounds-default.xml.in

EXTRA_DIST = $(sound_DATA) mate-sounds-default.xml.in.in

mate-sounds-default.xml.in: mate-sounds-default.xml.in.in Makefile
	$(AM_V_GEN)sed -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/mate-sounds-default.xml.in.in > mate-sounds-default.xml.in.tmp \
	&& mv mate-sounds-default.xml.in.tmp mate-sounds-default.xml.in

MAINTAINERCLEANFILES = \
	Makefile.in

-include $(top_srcdir)/git.mk
