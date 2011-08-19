NULL =

BUILT_SOURCES =                                         	\
        udisks-daemon-glue.h                      	\
        udisks-device-glue.h				\
        udisks-adapter-glue.h				\
        udisks-expander-glue.h				\
        udisks-port-glue.h				\
	gdu-marshal.h			gdu-marshal.c

gdu-marshal.h: gdu-marshal.list
	glib-genmarshal $< --prefix=gdu_marshal --header > $@

gdu-marshal.c: gdu-marshal.list
	echo "#include \"gdu-marshal.h\"" > $@ && glib-genmarshal $< --prefix=gdu_marshal --body >> $@

udisks-daemon-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.xml Makefile.am
	dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=udisks-daemon-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.xml

udisks-device-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Device.xml Makefile.am
	dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=udisks-device-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Device.xml

udisks-adapter-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Adapter.xml Makefile.am
	dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=udisks-adapter-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Adapter.xml

udisks-expander-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Expander.xml Makefile.am
	dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=udisks-expander-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Expander.xml

udisks-port-glue.h: /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Port.xml Makefile.am
	dbus-binding-tool --prefix=devkit_disks_daemon --mode=glib-client --output=udisks-port-glue.h /usr/share/dbus-1/interfaces/org.freedesktop.UDisks.Port.xml

lib_LTLIBRARIES=libgdu.la

libgduincludedir=$(includedir)/mate-disk-utility/gdu

libgduinclude_HEADERS =              			\
	gdu.h						\
	gdu-types.h					\
	gdu-callbacks.h					\
	gdu-device.h					\
	gdu-adapter.h					\
	gdu-expander.h					\
	gdu-port.h					\
	gdu-drive.h					\
	gdu-linux-md-drive.h				\
	gdu-linux-lvm2-volume-group.h			\
	gdu-linux-lvm2-volume-hole.h			\
	gdu-linux-lvm2-volume.h				\
	gdu-error.h					\
	gdu-known-filesystem.h				\
	gdu-pool.h					\
	gdu-presentable.h				\
	gdu-process.h					\
	gdu-util.h					\
	gdu-volume.h					\
	gdu-volume-hole.h				\
	gdu-hub.h					\
	gdu-machine.h					\
	$(NULL)

libgdu_la_SOURCES =                                					\
						gdu.h					\
						gdu-types.h				\
						gdu-callbacks.h				\
	gdu-util.h				gdu-util.c				\
	gdu-pool.c				gdu-pool.h				\
	gdu-device.c				gdu-device.h				\
	gdu-adapter.c				gdu-adapter.h				\
	gdu-expander.c				gdu-expander.h				\
	gdu-port.c				gdu-port.h				\
	gdu-drive.c				gdu-drive.h				\
	gdu-linux-md-drive.c			gdu-linux-md-drive.h			\
	gdu-linux-lvm2-volume-group.c		gdu-linux-lvm2-volume-group.h		\
	gdu-linux-lvm2-volume.c			gdu-linux-lvm2-volume.h			\
	gdu-linux-lvm2-volume-hole.c		gdu-linux-lvm2-volume-hole.h		\
	gdu-volume.c				gdu-volume.h				\
	gdu-presentable.c			gdu-presentable.h			\
	gdu-volume-hole.c			gdu-volume-hole.h			\
	gdu-known-filesystem.c			gdu-known-filesystem.h			\
	gdu-error.c				gdu-error.h				\
	gdu-process.c				gdu-process.h				\
	gdu-hub.c				gdu-hub.h				\
	gdu-machine.c				gdu-machine.h				\
						gdu-private.h				\
	gdu-ssh-bridge.c			gdu-ssh-bridge.h			\
	$(BUILT_SOURCES)								\
	$(NULL)

libgdu_la_CPPFLAGS = 					\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	-DG_LOG_DOMAIN=\"libgdu\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)					\
	-DGDU_API_IS_SUBJECT_TO_CHANGE			\
	-DGDU_COMPILATION

libgdu_la_CFLAGS = 					\
	$(GLIB2_CFLAGS)					\
	$(GOBJECT2_CFLAGS)				\
	$(GIO2_CFLAGS)					\
	$(GIO_UNIX2_CFLAGS)				\
	$(DBUS_GLIB_CFLAGS)				\
	$(MATE_KEYRING_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)

libgdu_la_LIBADD = 					\
	$(GLIB2_LIBS)					\
	$(GIO2_LIBS)					\
	$(GIO_UNIX2_LIBS)				\
	$(DBUS_GLIB_LIBS)				\
	$(MATE_KEYRING_LIBS)				\
	$(INTLLIBS)

libgdu_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		    -export-dynamic -no-undefined -export-symbols-regex '(^gdu_.*)'

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdu.pc

CLEANFILES = $(BUILT_SOURCES) $(pkgconfig_DATA)

EXTRA_DIST = gdu-marshal.list

clean-local :
	rm -f *~ $(BUILT_SOURCES)

