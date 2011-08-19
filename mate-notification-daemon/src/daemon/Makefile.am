libexec_PROGRAMS = mate-notification-daemon

mate_notification_daemon_SOURCES = \
	daemon.c \
	daemon.h \
	engines.c \
	engines.h \
	stack.c \
	stack.h \
	sound.c \
	sound.h

mate_notification_daemon_LDADD = $(NOTIFICATION_DAEMON_LIBS)

BUILT_SOURCES = notificationdaemon-dbus-glue.h

notificationdaemon-dbus-glue.h: notificationdaemon.xml
	dbus-binding-tool --mode=glib-server --prefix=notification_daemon \
		$(srcdir)/notificationdaemon.xml > notificationdaemon-dbus-glue.h

INCLUDES = \
	-I$(top_srcdir) \
	$(NOTIFICATION_DAEMON_CFLAGS) \
	-DENGINES_DIR=\"$(libdir)/mate-notification-daemon/engines\"

EXTRA_DIST = notificationdaemon.xml
DISTCLEANFILES = \
	notificationdaemon-dbus-glue.h

-include $(top_srcdir)/git.mk
