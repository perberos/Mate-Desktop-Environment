include $(top_srcdir)/Makefile.decl

pwd=`pwd`
INCLUDES = \
	-I$(top_srcdir)/plparse	\
	-DTEST_SRCDIR=\""$(srcdir)/"\"

noinst_PROGRAMS = $(TEST_PROGS)

TEST_PROGS			+= parser
parser_SOURCES			 = parser.c
parser_LDADD			 = \
				$(progs_ldadd)		\
				$(TOTEM_PLPARSER_LIBS)	\
				../libtotem-plparser.la
parser_CPPFLAGS			 = \
				-I$(top_srcdir)			\
				-I$(top_srcdir)/lib		\
				-I$(top_builddir)/plparse	\
				-DMATELOCALEDIR=\""$(datadir)/locale"\"\
				$(DISABLE_DEPRECATED)		\
				$(INCLUDES)			\
				$(AM_CPPFLAGS)
parser_CFLAGS			 = \
				$(TOTEM_PLPARSER_CFLAGS)\
				$(HAL_CFLAGS)		\
				$(DBUS_CFLAGS)		\
				$(WARN_CFLAGS)		\
				$(AM_CFLAGS)

TEST_PROGS			+= disc
disc_SOURCES			 = disc.c
disc_LDADD			 = \
				$(progs_ldadd)		\
				../libtotem-plparser.la	\
				$(TOTEM_PLPARSER_LIBS)
disc_CPPFLAGS			 = \
				$(common_defines)	\
				$(INCLUDES)		\
				$(AM_CPPFLAGS)
disc_CFLAGS			 = \
				$(WARN_CFLAGS)		\
				$(TOTEM_PLPARSER_CFLAGS)\
				$(AM_CFLAGS)
disc_LDFLAGS			 = $(AM_LDFLAGS)

EXTRA_DIST =			\
	541405.xml		\
	560051.xml		\
	585407.rss		\
	602127.qtl		\
	live-streaming.m3u	\
	single-line.qtl		\
	pukas.wax		\
	asf-with-asx-suffix.asx	\
	remote_xspf.php		\
	HackerMedley		\
	missing-items.pls	\
	playlist.xspf		\
	new-lastfm-output.xspf	\
	old-lastfm-output.xspf	\
	separator.m3u		\
	3gpp-file.mp4		\
	really-flv.mp4		\
	audio.php
