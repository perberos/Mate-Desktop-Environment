INCLUDES = \
	-I$(top_srcdir) 				\
	$(WARN_CFLAGS)					\
	$(LIBMATECOMPONENT_CFLAGS)				\
	-DPLUGIN_DIR=\""$(PLUGIN_DIR)"\"		\
        -DMATECOMPONENT_LIBDIR=\""$(libdir)"\" 		\
        -DMATECOMPONENT_DATADIR=\""$(datadir)"\" 		\
        -DMATECOMPONENT_PIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DMATECOMPONENT_BINDIR=\""$(bindir)"\" 		\
        -DMATECOMPONENT_LOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DMATECOMPONENT_LOCALEDIR=\""$(datadir)/locale"\" 	\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"

MateComponent_Moniker_std.server.in: MateComponent_Moniker_std.server.in.in
	$(AM_V_GEN) $(SED) -e "s|\@MONIKER_LIBDIR\@|$(monikerdir_in_server_file)|" -e "s|\@SOPREFIX\@|$(SOPREFIX)|" $< > $@

CLEANFILES = MateComponent_Moniker_std.server MateComponent_Moniker_std.server.in

serverdir = $(libdir)/matecomponent/servers
server_DATA = MateComponent_Moniker_std.server

@INTLTOOL_SERVER_RULE@

monikerdir = $(libdir)/matecomponent/monikers

if OS_WIN32
monikerdir_in_server_file = ../monikers
else
monikerdir_in_server_file = $(monikerdir)
endif

moniker_LTLIBRARIES = libmoniker_std_2.la

libmoniker_std_2_la_SOURCES =			\
	matecomponent-moniker-extender-stream.c  	\
	matecomponent-moniker-item.c			\
	matecomponent-moniker-new.c			\
	matecomponent-moniker-ior.c			\
	matecomponent-moniker-oaf.c			\
	matecomponent-moniker-cache.c			\
	matecomponent-stream-cache.h			\
	matecomponent-stream-cache.c			\
	matecomponent-moniker-query.c			\
	matecomponent-moniker-query.h			\
	matecomponent-moniker-std.h            	\
	matecomponent-moniker-std.c			

libmoniker_std_2_la_LDFLAGS = -module -avoid-version -no-undefined
libmoniker_std_2_la_LIBADD = $(top_builddir)/matecomponent/libmatecomponent-2.la \
	$(top_builddir)/matecomponent-activation/libmatecomponent-activation.la \
	$(LIBMATECOMPONENT_LIBS) \
	$(INTLLIBS)

EXTRA_DIST = MateComponent_Moniker_std.server.in.in
