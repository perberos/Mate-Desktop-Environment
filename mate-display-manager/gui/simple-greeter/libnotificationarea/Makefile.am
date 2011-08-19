NULL =

INCLUDES = \
	-I.					\
	-I..					\
	-DBINDIR=\"$(bindir)\"			\
	-DDATADIR=\"$(datadir)\"		\
	-DLIBDIR=\"$(libdir)\"			\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DLOGDIR=\"$(logdir)\"			\
	-DPIXMAPDIR=\"$(pixmapdir)\"		\
	-DSBINDIR=\"$(sbindir)\"		\
	$(GTK_CFLAGS)				\
	$(XLIB_CFLAGS)				\
	$(NULL)

noinst_LTLIBRARIES = 			\
	libnotificationarea.la		\
	$(NULL)

BUILT_SOURCES = 	\
	na-marshal.c 	\
	na-marshal.h	\
	$(NULL)

libnotificationarea_la_SOURCES =		\
	fixedtip.h				\
	fixedtip.c				\
	obox.c					\
	obox.h					\
	na-tray.c				\
	na-tray.h				\
	na-tray-child.c				\
	na-tray-child.h				\
	na-tray-manager.c			\
	na-tray-manager.h			\
	na-marshal.c				\
	na-marshal.h				\
	$(NULL)

libnotificationarea_la_LIBADD =			\
	$(GTK_LIBS)				\
	$(XLIB_LIBS)				\
	$(NULL)

libnotificationarea_la_LDFLAGS = 		\
	-export-dynamic 			\
	$(NULL)

EXTRA_DIST =					\
	na-marshal.list				\
	$(NULL)

na-marshal.h: na-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=_na_marshal > $@

na-marshal.c: na-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"na-marshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=_na_marshal >> $@

noinst_PROGRAMS = testtray

testtray_SOURCES = 		\
	testtray.c		\
	$(NULL)

testtray_LDADD = 		\
	libnotificationarea.la	\
	$(GREETER_LIBS)		\
	$(XLIB_LIBS)		\
	$(NULL)
