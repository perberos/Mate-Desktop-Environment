INCLUDES = 			\
	-I.			\
	-I$(srcdir)		\
	$(MATE_APPLETS_CFLAGS)

## The null applet takes over if the mixer isn't built.
if !BUILD_MIXER_APPLET
mixer_server_in = MATE_MixerApplet.server.in
endif

libexec_PROGRAMS = null_applet

null_applet_SOURCES = null_applet.c

null_applet_LDADD = $(MATE_APPLETS_LIBS)

serverdir = $(libdir)/matecomponent/servers
server_in_files =				\
	MATE_NullApplet_Factory.server.in	\
	MATE_CDPlayerApplet.server.in		\
	MATE_MailcheckApplet_Factory.server.in	\
	MATE_Panel_WirelessApplet.server.in	\
	MATE_KeyboardApplet.server.in		\
	$(mixer_server_in)
server_DATA = $(server_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

MATE_NullApplet_Factory.server.in: MATE_NullApplet_Factory.server.in.in
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

CLEANFILES = MATE_NullApplet_Factory.server.in $(server_DATA)

## Note that the mixer applet server.in file is always distributed, but
## note always used.
EXTRA_DIST =					\
	MATE_NullApplet_Factory.server.in.in	\
	MATE_CDPlayerApplet.server.in		\
	MATE_MailcheckApplet_Factory.server.in	\
	MATE_Panel_WirelessApplet.server.in	\
	MATE_MixerApplet.server.in		\
	MATE_KeyboardApplet.server.in

-include $(top_srcdir)/git.mk
