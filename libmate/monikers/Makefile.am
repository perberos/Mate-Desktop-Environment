INCLUDES = \
	-I$(top_srcdir) \
	$(LIBMATE_CFLAGS) \
	-DG_LOG_DOMAIN=\"libmate\" \
	-DVERSION=\"$(VERSION)\"

MATE_Moniker_std.server.in: MATE_Moniker_std.server.in.in
	$(AM_V_GEN) $(SED) -e "s|\@MONIKER_LIBDIR\@|$(monikerdir_in_server_file)|" \
		-e "s|\@SOPREFIX\@|$(SOPREFIX)|" $< > $@

CLEANFILES = MATE_Moniker_std.server MATE_Moniker_std.server.in

serverdir = $(libdir)/matecomponent/servers
server_DATA = MATE_Moniker_std.server

@INTLTOOL_SERVER_RULE@

monikerdir = $(libdir)/matecomponent/monikers

if OS_WIN32
monikerdir_in_server_file = ../monikers
else
monikerdir_in_server_file = $(monikerdir)
endif

moniker_LTLIBRARIES = libmoniker_extra_2.la

libmoniker_extra_2_la_SOURCES = \
	matecomponent-config-bag.c \
	matecomponent-config-bag.h \
	matecomponent-moniker-config.c \
	matecomponent-moniker-conf-indirect.c \
	matecomponent-moniker-extra.h \
	matecomponent-moniker-extra.c

libmoniker_extra_2_la_LDFLAGS = -module -avoid-version -no-undefined

libmoniker_extra_2_la_LIBADD = $(LIBMATE_LIBS)

EXTRA_DIST = MATE_Moniker_std.server.in.in
