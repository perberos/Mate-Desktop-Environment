NULL =

AM_CPPFLAGS = \
	-I$(top_srcdir)/gui/simple-greeter		\
	-I$(top_srcdir)/common				\
	-DPREFIX=\""$(prefix)"\" 			\
	-DLIBDIR=\""$(libdir)"\" 			\
	-DDATADIR=\""$(datadir)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBLOCALEDIR=\""$(prefix)/lib/locale"\"	\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGLADEDIR=\""$(pkgdatadir)"\" 			\
	-DLIBEXECDIR=\""$(libexecdir)"\" 		\
	-DSBINDIR=\""$(sbindir)"\"		 	\
	-DMDMCONFDIR=\"$(mdmconfdir)\"			\
	$(GTK_CFLAGS)					\
	$(APPLET_CFLAGS)				\
	$(NULL)

libexec_PROGRAMS =			\
	mdm-user-switch-applet		\
	$(NULL)

mdm_user_switch_applet_SOURCES =  	\
	applet.c 			\
	mdm-entry-menu-item.h		\
	mdm-entry-menu-item.c		\
	$(NULL)

mdm_user_switch_applet_LDADD = 		\
	$(top_builddir)/gui/simple-greeter/libmdmuser.la	\
	$(top_builddir)/common/libmdmcommon.la		\
	$(COMMON_LIBS)					\
	$(APPLET_LIBS)					\
	$(NULL)

# MateComponent UI
uidir =	$(datadir)/mate-2.0/ui
ui_DATA =					\
	MATE_FastUserSwitchApplet.xml		\
	$(NULL)

# MateComponent .server
serverdir = $(libdir)/matecomponent/servers
server_in_files =				\
	MATE_FastUserSwitchApplet.server.in	\
	$(NULL)
server_DATA = $(server_in_files:.server.in=.server)

$(server_in_files): $(server_in_files:.server.in=.server.in.in)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" -e "s|\@VERSION\@|$(VERSION)|" $< > $@

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = 			\
	$(ui_DATA)		\
	MATE_FastUserSwitchApplet.server.in.in	\
	$(NULL)

DISTCLEANFILES =                        \
	$(server_in_files)		\
	$(server_DATA)			\
        $(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in
