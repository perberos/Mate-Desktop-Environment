INCLUDES= \
	$(LIBMATEVFS_CFLAGS)			\
	$(LIBMATEVFSDAEMON_CFLAGS)		\
	$(VFS_CFLAGS)				\
	-DDBUS_API_SUBJECT_TO_CHANGE		\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_LARGEFILE64_SOURCE                   \
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DG_DISABLE_DEPRECATED			\
	-DMATE_VFS_PREFIX=\"$(prefix)\"	\
	-DMATE_VFS_DATADIR=\"$(datadir)\"	\
	-DMATE_VFS_LIBDIR=\"$(libdir)\"	\
	-DMATEVFS_LOCALEDIR=\"$(prefix)/@DATADIRNAME@/locale\" \
	-DMATE_VFS_SYSCONFDIR=\"$(sysconfdir)\"\
	-DG_LOG_DOMAIN=\"libmatevfs\"		\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/libmatevfs             \
	-I$(top_builddir)/libmatevfs           \
	$(NULL)

if OS_WIN32
else
libexec_PROGRAMS=mate-vfs-daemon
endif

mate_vfs_daemon_LDADD = 		\
	$(LIBMATEVFS_LIBS)		\
	$(LIBMATEVFSDAEMON_LIBS)	\
	$(top_builddir)/libmatevfs/libmatevfsdaemon-2.la \
	$(top_builddir)/libmatevfs/libmatevfs-2.la

mate_vfs_daemon_SOURCES = \
	dbus-utils.c			\
	dbus-utils.h			\
	vfs-daemon.c			\
	daemon-connection.c		\
	daemon-connection.h

# D-BUS service file
%.service: %.service.in ../config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

servicedir = $(DBUS_SERVICE_DIR)
service_in_files = mate-vfs-daemon.service.in
service_DATA = mate-vfs-daemon.service

EXTRA_DIST = $(modulesconf_DATA) mate-vfs-daemon.service.in

DISTCLEANFILES = mate-vfs-daemon.service



