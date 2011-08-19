
NULL =

bin_PROGRAMS = palimpsest

palimpsest_SOURCES = 									\
						gdu-main.c				\
	gdu-shell.h				gdu-shell.c				\
	gdu-section.h				gdu-section.c				\
	gdu-section-linux-md-drive.h		gdu-section-linux-md-drive.c		\
	gdu-section-linux-lvm2-volume-group.h	gdu-section-linux-lvm2-volume-group.c	\
	gdu-section-drive.h			gdu-section-drive.c			\
	gdu-section-volumes.h			gdu-section-volumes.c			\
	gdu-section-hub.h			gdu-section-hub.c			\
	$(NULL)

palimpsest_CPPFLAGS = 					\
	-I$(top_srcdir)/src/				\
	-I$(top_builddir)/src/				\
	-DG_LOG_DOMAIN=\"Palimpsest\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	-DGDU_API_IS_SUBJECT_TO_CHANGE			\
	-DGDU_GTK_API_IS_SUBJECT_TO_CHANGE		\
	$(AM_CPPFLAGS)

palimpsest_CFLAGS = 					\
	$(GLIB2_CFLAGS)					\
	$(GOBJECT2_CFLAGS)				\
	$(GIO2_CFLAGS)					\
	$(GIO_UNIX2_CFLAGS)				\
	$(DBUS_GLIB_CFLAGS)				\
	$(MATE_KEYRING_CFLAGS)				\
	$(GTK2_CFLAGS)					\
	$(UNIQUE_CFLAGS)				\
	$(X11_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)

palimpsest_LDFLAGS = 					\
	$(AM_LDFLAGS)

palimpsest_LDADD = 					\
	$(GLIB2_LIBS)					\
	$(GIO2_LIBS)					\
	$(GIO_UNIX2_LIBS)				\
	$(DBUS_GLIB_LIBS)				\
	$(MATE_KEYRING_LIBS)				\
	$(GTK2_LIBS)					\
	$(UNIQUE_LIBS)					\
	$(X11_LIBS)					\
	$(INTLLIBS)					\
	$(top_builddir)/src/gdu/libgdu.la		\
	$(top_builddir)/src/gdu-gtk/libgdu-gtk.la

clean-local :
	rm -f *~
