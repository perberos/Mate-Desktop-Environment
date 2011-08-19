
NULL =

libexec_PROGRAMS = gdu-notification-daemon

gdu_notification_daemon_SOURCES = 							\
						notification-main.c			\
	gdu-slow-unmount-dialog.h		gdu-slow-unmount-dialog.c		\
	$(NULL)

gdu_notification_daemon_CPPFLAGS = 			\
	-I$(top_srcdir)/src/				\
	-I$(top_builddir)/src/				\
	-DG_LOG_DOMAIN=\"GduNotification\"		\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	-DGDU_API_IS_SUBJECT_TO_CHANGE			\
	-DGDU_GTK_API_IS_SUBJECT_TO_CHANGE		\
	$(AM_CPPFLAGS)

gdu_notification_daemon_CFLAGS = 			\
	$(GLIB2_CFLAGS)					\
	$(GOBJECT2_CFLAGS)				\
	$(GIO2_CFLAGS)					\
	$(GIO_UNIX2_CFLAGS)				\
	$(GTK2_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(LIBMATENOTIFY_CFLAGS)				\
	$(AM_CFLAGS)

gdu_notification_daemon_LDFLAGS = 			\
	$(AM_LDFLAGS)

gdu_notification_daemon_LDADD = 			\
	$(GLIB2_LIBS)					\
	$(GOBJECT2_LIBS)				\
	$(GIO2_LIBS)					\
	$(GTK2_LIBS)					\
	$(LIBMATENOTIFY_LIBS)				\
	$(top_builddir)/src/gdu/libgdu.la		\
	$(top_builddir)/src/gdu-gtk/libgdu-gtk.la

clean-local :
	rm -f *~
