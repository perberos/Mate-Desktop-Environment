NULL =

AM_CPPFLAGS = \
	-I.						\
	-I..						\
	-I$(top_srcdir)/common				\
	-I$(top_builddir)/common			\
	-DAUTHDIR=\"$(authdir)\"			\
	-DBINDIR=\"$(bindir)\"				\
	-DDATADIR=\"$(datadir)\"			\
	-DDMCONFDIR=\"$(dmconfdir)\"			\
	-DGDMCONFDIR=\"$(gdmconfdir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DLOGDIR=\"$(logdir)\"				\
	-DSBINDIR=\"$(sbindir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DGDM_XAUTH_DIR=\"$(GDM_XAUTH_DIR)\"		\
	-DGDM_SCREENSHOT_DIR=\"$(GDM_SCREENSHOT_DIR)\"		\
	-DGDM_CACHE_DIR=\""$(localstatedir)/cache/gdm"\"	\
	-DGDM_SESSION_DEFAULT_PATH=\"$(GDM_SESSION_DEFAULT_PATH)\" \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(DAEMON_CFLAGS)				\
	$(XLIB_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(DEBUG_CFLAGS)					\
	-DLANG_CONFIG_FILE=\"$(LANG_CONFIG_FILE)\"	\
	$(NULL)

BUILT_SOURCES =					\
	gdm-slave-glue.h			\
	gdm-simple-slave-glue.h			\
	gdm-factory-slave-glue.h		\
	gdm-product-slave-glue.h		\
	gdm-xdmcp-chooser-slave-glue.h		\
	gdm-session-direct-glue.h		\
	gdm-manager-glue.h			\
	gdm-display-glue.h			\
	gdm-xdmcp-greeter-display-glue.h	\
	gdm-xdmcp-chooser-display-glue.h	\
	gdm-static-display-glue.h		\
	gdm-transient-display-glue.h		\
	gdm-local-display-factory-glue.h	\
	gdm-static-factory-display-glue.h	\
	gdm-product-display-glue.h		\
	$(NULL)

gdm-manager-glue.h: gdm-manager.xml Makefile.am
	dbus-binding-tool --prefix=gdm_manager --mode=glib-server --output=gdm-manager-glue.h $(srcdir)/gdm-manager.xml
gdm-slave-glue.h: gdm-slave.xml Makefile.am
	dbus-binding-tool --prefix=gdm_slave --mode=glib-server --output=gdm-slave-glue.h $(srcdir)/gdm-slave.xml
gdm-simple-slave-glue.h: gdm-simple-slave.xml Makefile.am
	dbus-binding-tool --prefix=gdm_simple_slave --mode=glib-server --output=gdm-simple-slave-glue.h $(srcdir)/gdm-simple-slave.xml
gdm-factory-slave-glue.h: gdm-factory-slave.xml Makefile.am
	dbus-binding-tool --prefix=gdm_factory_slave --mode=glib-server --output=gdm-factory-slave-glue.h $(srcdir)/gdm-factory-slave.xml
gdm-product-slave-glue.h: gdm-product-slave.xml Makefile.am
	dbus-binding-tool --prefix=gdm_product_slave --mode=glib-server --output=gdm-product-slave-glue.h $(srcdir)/gdm-product-slave.xml
gdm-xdmcp-chooser-slave-glue.h: gdm-xdmcp-chooser-slave.xml Makefile.am
	dbus-binding-tool --prefix=gdm_xdmcp_chooser_slave --mode=glib-server --output=gdm-xdmcp-chooser-slave-glue.h $(srcdir)/gdm-xdmcp-chooser-slave.xml
gdm-session-direct-glue.h: gdm-session-direct.xml Makefile.am
	dbus-binding-tool --prefix=gdm_session_direct --mode=glib-server --output=gdm-session-direct-glue.h $(srcdir)/gdm-session-direct.xml
gdm-display-glue.h: gdm-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_display --mode=glib-server --output=gdm-display-glue.h $(srcdir)/gdm-display.xml
gdm-xdmcp-greeter-display-glue.h: gdm-xdmcp-greeter-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_xdmcp_greeter_display --mode=glib-server --output=gdm-xdmcp-greeter-display-glue.h $(srcdir)/gdm-xdmcp-greeter-display.xml
gdm-xdmcp-chooser-display-glue.h: gdm-xdmcp-chooser-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_xdmcp_chooser_display --mode=glib-server --output=gdm-xdmcp-chooser-display-glue.h $(srcdir)/gdm-xdmcp-chooser-display.xml
gdm-static-display-glue.h: gdm-static-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_static_display --mode=glib-server --output=gdm-static-display-glue.h $(srcdir)/gdm-static-display.xml
gdm-transient-display-glue.h: gdm-transient-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_transient_display --mode=glib-server --output=gdm-transient-display-glue.h $(srcdir)/gdm-transient-display.xml
gdm-local-display-factory-glue.h: gdm-local-display-factory.xml Makefile.am
	dbus-binding-tool --prefix=gdm_local_display_factory --mode=glib-server --output=gdm-local-display-factory-glue.h $(srcdir)/gdm-local-display-factory.xml
gdm-static-factory-display-glue.h: gdm-static-factory-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_static_factory_display --mode=glib-server --output=gdm-static-factory-display-glue.h $(srcdir)/gdm-static-factory-display.xml
gdm-product-display-glue.h: gdm-product-display.xml Makefile.am
	dbus-binding-tool --prefix=gdm_product_display --mode=glib-server --output=gdm-product-display-glue.h $(srcdir)/gdm-product-display.xml

noinst_PROGRAMS = 		\
	test-session		\
	$(NULL)

test_session_SOURCES = 		\
	test-session.c	 	\
	gdm-session.c		\
	gdm-session.h		\
	gdm-session-private.h	\
	gdm-session-direct.c	\
	gdm-session-direct.h	\
	gdm-session-record.c	\
	gdm-session-record.h	\
	gdm-session-worker-job.c\
	$(NULL)

test_session_LDADD =		\
	$(top_builddir)/common/libgdmcommon.la	\
	$(XLIB_LIBS)		\
	$(EXTRA_DAEMON_LIBS)	\
	$(DAEMON_LIBS)		\
	$(PAM_LIBS)		\
	$(LIBXKLAVIER_LIBS)	\
	$(NULL)

libexec_PROGRAMS = 			\
	gdm-simple-slave		\
	gdm-factory-slave		\
	gdm-product-slave		\
	gdm-session-worker		\
	$(NULL)

if XDMCP_SUPPORT
libexec_PROGRAMS += 			\
	gdm-xdmcp-chooser-slave		\
	$(NULL)
endif

gdm_simple_slave_SOURCES = 		\
	simple-slave-main.c 		\
	gdm-greeter-server.c		\
	gdm-greeter-server.h		\
	ck-connector.h			\
	ck-connector.c			\
	gdm-welcome-session.c		\
	gdm-welcome-session.h		\
	gdm-greeter-session.c		\
	gdm-greeter-session.h		\
	gdm-server.c			\
	gdm-server.h			\
	gdm-session.c			\
	gdm-session.h			\
	gdm-session-private.h		\
	gdm-session-direct.c		\
	gdm-session-direct.h		\
	gdm-session-record.c		\
	gdm-session-record.h		\
	gdm-session-worker-job.c	\
	gdm-session-worker-job.h	\
	gdm-xerrors.h			\
	gdm-xerrors.c			\
	gdm-slave.c			\
	gdm-slave.h			\
	gdm-simple-slave.c		\
	gdm-simple-slave.h		\
	$(NULL)

gdm_simple_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

gdm_simple_slave_LDADD = 			\
	$(top_builddir)/common/libgdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(LIBXKLAVIER_LIBS)			\
	$(NULL)

gdm_factory_slave_SOURCES = 		\
	factory-slave-main.c 		\
	gdm-greeter-server.c		\
	gdm-greeter-server.h		\
	ck-connector.h			\
	ck-connector.c			\
	gdm-welcome-session.c		\
	gdm-welcome-session.h		\
	gdm-greeter-session.c		\
	gdm-greeter-session.h		\
	gdm-server.c			\
	gdm-server.h			\
	gdm-session.c			\
	gdm-session.h			\
	gdm-session-private.h		\
	gdm-session-relay.c		\
	gdm-session-relay.h		\
	gdm-xerrors.h			\
	gdm-xerrors.c			\
	gdm-slave.c			\
	gdm-slave.h			\
	gdm-factory-slave.c		\
	gdm-factory-slave.h		\
	$(NULL)

gdm_factory_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

gdm_factory_slave_LDADD = 			\
	$(top_builddir)/common/libgdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(NULL)

gdm_product_slave_SOURCES = 		\
	product-slave-main.c 		\
	gdm-server.c			\
	gdm-server.h			\
	gdm-session.c			\
	gdm-session.h			\
	gdm-session-private.h		\
	gdm-session-direct.c		\
	gdm-session-direct.h		\
	gdm-session-record.c		\
	gdm-session-record.h		\
	gdm-session-worker-job.c	\
	gdm-session-worker-job.h	\
	gdm-xerrors.h			\
	gdm-xerrors.c			\
	gdm-slave.c			\
	gdm-slave.h			\
	gdm-product-slave.c		\
	gdm-product-slave.h		\
	$(NULL)

gdm_product_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

gdm_product_slave_LDADD = 			\
	$(top_builddir)/common/libgdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(LIBXKLAVIER_LIBS)			\
	$(NULL)

gdm_xdmcp_chooser_slave_SOURCES = 		\
	xdmcp-chooser-slave-main.c 		\
	gdm-chooser-server.c			\
	gdm-chooser-server.h			\
	ck-connector.h				\
	ck-connector.c				\
	gdm-welcome-session.c			\
	gdm-welcome-session.h			\
	gdm-chooser-session.c			\
	gdm-chooser-session.h			\
	gdm-xerrors.h				\
	gdm-xerrors.c				\
	gdm-slave.c				\
	gdm-slave.h				\
	gdm-xdmcp-chooser-slave.c		\
	gdm-xdmcp-chooser-slave.h		\
	$(NULL)

gdm_xdmcp_chooser_slave_LDADD = 		\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(top_builddir)/common/libgdmcommon.la	\
	$(NULL)

gdm_session_worker_SOURCES = 			\
	session-worker-main.c 			\
	ck-connector.h				\
	ck-connector.c				\
	gdm-session-settings.h			\
	gdm-session-settings.c			\
	gdm-session-auditor.h			\
	gdm-session-auditor.c			\
	gdm-session-worker.h			\
	gdm-session-worker.c			\
	$(NULL)

if HAVE_LIBAUDIT
gdm_session_worker_SOURCES += gdm-session-linux-auditor.h       \
	                      gdm-session-linux-auditor.c
endif

if HAVE_ADT
gdm_session_worker_SOURCES += gdm-session-solaris-auditor.h     \
	                      gdm-session-solaris-auditor.c
endif

gdm_session_worker_LDFLAGS =			\
	$(XLIB_LIBS)				\
	$(PAM_LIBS)				\
	$(NULL)

gdm_session_worker_LDADD = 			\
	$(top_builddir)/common/libgdmcommon.la	\
	$(DAEMON_LIBS)				\
	$(NULL)

sbin_PROGRAMS = 			\
	gdm-binary			\
	$(NULL)

gdm_binary_SOURCES = 			\
	main.c				\
	gdm-display-access-file.c	\
	gdm-display-access-file.h	\
	gdm-display-store.c		\
	gdm-display-store.h		\
	gdm-display-factory.c		\
	gdm-display-factory.h		\
	gdm-local-display-factory.c	\
	gdm-local-display-factory.h	\
	gdm-display.c			\
	gdm-display.h			\
	gdm-static-display.c		\
	gdm-static-display.h		\
	gdm-transient-display.c		\
	gdm-transient-display.h		\
	gdm-static-factory-display.c	\
	gdm-static-factory-display.h	\
	gdm-product-display.c		\
	gdm-product-display.h		\
	gdm-manager.c			\
	gdm-manager.h			\
	gdm-slave-proxy.c		\
	gdm-slave-proxy.h		\
	$(NULL)

XDMCP_SOURCES =				\
	gdm-xdmcp-display-factory.c	\
	gdm-xdmcp-display-factory.h	\
	gdm-xdmcp-display.c		\
	gdm-xdmcp-display.h		\
	gdm-xdmcp-greeter-display.c	\
	gdm-xdmcp-greeter-display.h	\
	gdm-xdmcp-chooser-display.c	\
	gdm-xdmcp-chooser-display.h	\
	$(NULL)

if XDMCP_SUPPORT
gdm_binary_SOURCES += $(XDMCP_SOURCES)
endif

EXTRA_gdm_binary_SOURCES = 	\
	$(XDMCP_SOURCES)	\
	$(NULL)

CONSOLE_KIT_SOURCES = 		\
	$(NULL)

EXTRA_gdm_binary_SOURCES += $(CONSOLE_KIT_SOURCES)

# Note that these libs are in LDFLAGS because they should come before
# everything else on the link line as they may override stuff
gdm_binary_LDFLAGS =				\
	$(EXTRA_DAEMON_LIBS)			\
	$(NULL)

gdm_binary_LDADD = \
	$(top_builddir)/common/libgdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(XDMCP_LIBS)                           \
	$(LIBWRAP_LIBS)                         \
	$(NULL)

if WITH_CONSOLE_KIT
gdm_binary_SOURCES += $(CONSOLE_KIT_SOURCES)
endif

sbin_SCRIPTS =		\
	gdm		\
	$(NULL)

gdm: $(srcdir)/gdm.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' <$(srcdir)/gdm.in >gdm

CLEANFILES =			\
	gdm			\
	$(BUILT_SOURCES)	\
	$(NULL)

EXTRA_DIST = 				\
	gdm.in				\
	gdm-slave.xml			\
	gdm-simple-slave.xml		\
	gdm-factory-slave.xml		\
	gdm-product-slave.xml		\
	gdm-xdmcp-chooser-slave.xml	\
	gdm-session-direct.xml		\
	gdm-manager.xml			\
	gdm-display.xml			\
	gdm-xdmcp-greeter-display.xml	\
	gdm-xdmcp-chooser-display.xml	\
	gdm-static-display.xml		\
	gdm-transient-display.xml	\
	gdm-local-display-factory.xml	\
	gdm-static-factory-display.xml	\
	gdm-product-display.xml		\
	$(NULL)
