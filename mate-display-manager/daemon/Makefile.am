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
	-DMDMCONFDIR=\"$(mdmconfdir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-DLOGDIR=\"$(logdir)\"				\
	-DSBINDIR=\"$(sbindir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMDM_XAUTH_DIR=\"$(MDM_XAUTH_DIR)\"		\
	-DMDM_SCREENSHOT_DIR=\"$(MDM_SCREENSHOT_DIR)\"		\
	-DMDM_CACHE_DIR=\""$(localstatedir)/cache/mdm"\"	\
	-DMDM_SESSION_DEFAULT_PATH=\"$(MDM_SESSION_DEFAULT_PATH)\" \
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(DAEMON_CFLAGS)				\
	$(XLIB_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(DEBUG_CFLAGS)					\
	-DLANG_CONFIG_FILE=\"$(LANG_CONFIG_FILE)\"	\
	$(NULL)

BUILT_SOURCES =					\
	mdm-slave-glue.h			\
	mdm-simple-slave-glue.h			\
	mdm-factory-slave-glue.h		\
	mdm-product-slave-glue.h		\
	mdm-xdmcp-chooser-slave-glue.h		\
	mdm-session-direct-glue.h		\
	mdm-manager-glue.h			\
	mdm-display-glue.h			\
	mdm-xdmcp-greeter-display-glue.h	\
	mdm-xdmcp-chooser-display-glue.h	\
	mdm-static-display-glue.h		\
	mdm-transient-display-glue.h		\
	mdm-local-display-factory-glue.h	\
	mdm-static-factory-display-glue.h	\
	mdm-product-display-glue.h		\
	$(NULL)

mdm-manager-glue.h: mdm-manager.xml Makefile.am
	dbus-binding-tool --prefix=mdm_manager --mode=glib-server --output=mdm-manager-glue.h $(srcdir)/mdm-manager.xml
mdm-slave-glue.h: mdm-slave.xml Makefile.am
	dbus-binding-tool --prefix=mdm_slave --mode=glib-server --output=mdm-slave-glue.h $(srcdir)/mdm-slave.xml
mdm-simple-slave-glue.h: mdm-simple-slave.xml Makefile.am
	dbus-binding-tool --prefix=mdm_simple_slave --mode=glib-server --output=mdm-simple-slave-glue.h $(srcdir)/mdm-simple-slave.xml
mdm-factory-slave-glue.h: mdm-factory-slave.xml Makefile.am
	dbus-binding-tool --prefix=mdm_factory_slave --mode=glib-server --output=mdm-factory-slave-glue.h $(srcdir)/mdm-factory-slave.xml
mdm-product-slave-glue.h: mdm-product-slave.xml Makefile.am
	dbus-binding-tool --prefix=mdm_product_slave --mode=glib-server --output=mdm-product-slave-glue.h $(srcdir)/mdm-product-slave.xml
mdm-xdmcp-chooser-slave-glue.h: mdm-xdmcp-chooser-slave.xml Makefile.am
	dbus-binding-tool --prefix=mdm_xdmcp_chooser_slave --mode=glib-server --output=mdm-xdmcp-chooser-slave-glue.h $(srcdir)/mdm-xdmcp-chooser-slave.xml
mdm-session-direct-glue.h: mdm-session-direct.xml Makefile.am
	dbus-binding-tool --prefix=mdm_session_direct --mode=glib-server --output=mdm-session-direct-glue.h $(srcdir)/mdm-session-direct.xml
mdm-display-glue.h: mdm-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_display --mode=glib-server --output=mdm-display-glue.h $(srcdir)/mdm-display.xml
mdm-xdmcp-greeter-display-glue.h: mdm-xdmcp-greeter-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_xdmcp_greeter_display --mode=glib-server --output=mdm-xdmcp-greeter-display-glue.h $(srcdir)/mdm-xdmcp-greeter-display.xml
mdm-xdmcp-chooser-display-glue.h: mdm-xdmcp-chooser-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_xdmcp_chooser_display --mode=glib-server --output=mdm-xdmcp-chooser-display-glue.h $(srcdir)/mdm-xdmcp-chooser-display.xml
mdm-static-display-glue.h: mdm-static-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_static_display --mode=glib-server --output=mdm-static-display-glue.h $(srcdir)/mdm-static-display.xml
mdm-transient-display-glue.h: mdm-transient-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_transient_display --mode=glib-server --output=mdm-transient-display-glue.h $(srcdir)/mdm-transient-display.xml
mdm-local-display-factory-glue.h: mdm-local-display-factory.xml Makefile.am
	dbus-binding-tool --prefix=mdm_local_display_factory --mode=glib-server --output=mdm-local-display-factory-glue.h $(srcdir)/mdm-local-display-factory.xml
mdm-static-factory-display-glue.h: mdm-static-factory-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_static_factory_display --mode=glib-server --output=mdm-static-factory-display-glue.h $(srcdir)/mdm-static-factory-display.xml
mdm-product-display-glue.h: mdm-product-display.xml Makefile.am
	dbus-binding-tool --prefix=mdm_product_display --mode=glib-server --output=mdm-product-display-glue.h $(srcdir)/mdm-product-display.xml

noinst_PROGRAMS = 		\
	test-session		\
	$(NULL)

test_session_SOURCES = 		\
	test-session.c	 	\
	mdm-session.c		\
	mdm-session.h		\
	mdm-session-private.h	\
	mdm-session-direct.c	\
	mdm-session-direct.h	\
	mdm-session-record.c	\
	mdm-session-record.h	\
	mdm-session-worker-job.c\
	$(NULL)

test_session_LDADD =		\
	$(top_builddir)/common/libmdmcommon.la	\
	$(XLIB_LIBS)		\
	$(EXTRA_DAEMON_LIBS)	\
	$(DAEMON_LIBS)		\
	$(PAM_LIBS)		\
	$(LIBXKLAVIER_LIBS)	\
	$(NULL)

libexec_PROGRAMS = 			\
	mdm-simple-slave		\
	mdm-factory-slave		\
	mdm-product-slave		\
	mdm-session-worker		\
	$(NULL)

if XDMCP_SUPPORT
libexec_PROGRAMS += 			\
	mdm-xdmcp-chooser-slave		\
	$(NULL)
endif

mdm_simple_slave_SOURCES = 		\
	simple-slave-main.c 		\
	mdm-greeter-server.c		\
	mdm-greeter-server.h		\
	ck-connector.h			\
	ck-connector.c			\
	mdm-welcome-session.c		\
	mdm-welcome-session.h		\
	mdm-greeter-session.c		\
	mdm-greeter-session.h		\
	mdm-server.c			\
	mdm-server.h			\
	mdm-session.c			\
	mdm-session.h			\
	mdm-session-private.h		\
	mdm-session-direct.c		\
	mdm-session-direct.h		\
	mdm-session-record.c		\
	mdm-session-record.h		\
	mdm-session-worker-job.c	\
	mdm-session-worker-job.h	\
	mdm-xerrors.h			\
	mdm-xerrors.c			\
	mdm-slave.c			\
	mdm-slave.h			\
	mdm-simple-slave.c		\
	mdm-simple-slave.h		\
	$(NULL)

mdm_simple_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

mdm_simple_slave_LDADD = 			\
	$(top_builddir)/common/libmdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(LIBXKLAVIER_LIBS)			\
	$(NULL)

mdm_factory_slave_SOURCES = 		\
	factory-slave-main.c 		\
	mdm-greeter-server.c		\
	mdm-greeter-server.h		\
	ck-connector.h			\
	ck-connector.c			\
	mdm-welcome-session.c		\
	mdm-welcome-session.h		\
	mdm-greeter-session.c		\
	mdm-greeter-session.h		\
	mdm-server.c			\
	mdm-server.h			\
	mdm-session.c			\
	mdm-session.h			\
	mdm-session-private.h		\
	mdm-session-relay.c		\
	mdm-session-relay.h		\
	mdm-xerrors.h			\
	mdm-xerrors.c			\
	mdm-slave.c			\
	mdm-slave.h			\
	mdm-factory-slave.c		\
	mdm-factory-slave.h		\
	$(NULL)

mdm_factory_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

mdm_factory_slave_LDADD = 			\
	$(top_builddir)/common/libmdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(NULL)

mdm_product_slave_SOURCES = 		\
	product-slave-main.c 		\
	mdm-server.c			\
	mdm-server.h			\
	mdm-session.c			\
	mdm-session.h			\
	mdm-session-private.h		\
	mdm-session-direct.c		\
	mdm-session-direct.h		\
	mdm-session-record.c		\
	mdm-session-record.h		\
	mdm-session-worker-job.c	\
	mdm-session-worker-job.h	\
	mdm-xerrors.h			\
	mdm-xerrors.c			\
	mdm-slave.c			\
	mdm-slave.h			\
	mdm-product-slave.c		\
	mdm-product-slave.h		\
	$(NULL)

mdm_product_slave_LDFLAGS =		\
	$(PAM_LIBS)			\
	$(NULL)

mdm_product_slave_LDADD = 			\
	$(top_builddir)/common/libmdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(LIBXKLAVIER_LIBS)			\
	$(NULL)

mdm_xdmcp_chooser_slave_SOURCES = 		\
	xdmcp-chooser-slave-main.c 		\
	mdm-chooser-server.c			\
	mdm-chooser-server.h			\
	ck-connector.h				\
	ck-connector.c				\
	mdm-welcome-session.c			\
	mdm-welcome-session.h			\
	mdm-chooser-session.c			\
	mdm-chooser-session.h			\
	mdm-xerrors.h				\
	mdm-xerrors.c				\
	mdm-slave.c				\
	mdm-slave.h				\
	mdm-xdmcp-chooser-slave.c		\
	mdm-xdmcp-chooser-slave.h		\
	$(NULL)

mdm_xdmcp_chooser_slave_LDADD = 		\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(EXTRA_DAEMON_LIBS)                    \
	$(top_builddir)/common/libmdmcommon.la	\
	$(NULL)

mdm_session_worker_SOURCES = 			\
	session-worker-main.c 			\
	ck-connector.h				\
	ck-connector.c				\
	mdm-session-settings.h			\
	mdm-session-settings.c			\
	mdm-session-auditor.h			\
	mdm-session-auditor.c			\
	mdm-session-worker.h			\
	mdm-session-worker.c			\
	$(NULL)

if HAVE_LIBAUDIT
mdm_session_worker_SOURCES += mdm-session-linux-auditor.h       \
	                      mdm-session-linux-auditor.c
endif

if HAVE_ADT
mdm_session_worker_SOURCES += mdm-session-solaris-auditor.h     \
	                      mdm-session-solaris-auditor.c
endif

mdm_session_worker_LDFLAGS =			\
	$(XLIB_LIBS)				\
	$(PAM_LIBS)				\
	$(NULL)

mdm_session_worker_LDADD = 			\
	$(top_builddir)/common/libmdmcommon.la	\
	$(DAEMON_LIBS)				\
	$(NULL)

sbin_PROGRAMS = 			\
	mdm-binary			\
	$(NULL)

mdm_binary_SOURCES = 			\
	main.c				\
	mdm-display-access-file.c	\
	mdm-display-access-file.h	\
	mdm-display-store.c		\
	mdm-display-store.h		\
	mdm-display-factory.c		\
	mdm-display-factory.h		\
	mdm-local-display-factory.c	\
	mdm-local-display-factory.h	\
	mdm-display.c			\
	mdm-display.h			\
	mdm-static-display.c		\
	mdm-static-display.h		\
	mdm-transient-display.c		\
	mdm-transient-display.h		\
	mdm-static-factory-display.c	\
	mdm-static-factory-display.h	\
	mdm-product-display.c		\
	mdm-product-display.h		\
	mdm-manager.c			\
	mdm-manager.h			\
	mdm-slave-proxy.c		\
	mdm-slave-proxy.h		\
	$(NULL)

XDMCP_SOURCES =				\
	mdm-xdmcp-display-factory.c	\
	mdm-xdmcp-display-factory.h	\
	mdm-xdmcp-display.c		\
	mdm-xdmcp-display.h		\
	mdm-xdmcp-greeter-display.c	\
	mdm-xdmcp-greeter-display.h	\
	mdm-xdmcp-chooser-display.c	\
	mdm-xdmcp-chooser-display.h	\
	$(NULL)

if XDMCP_SUPPORT
mdm_binary_SOURCES += $(XDMCP_SOURCES)
endif

EXTRA_mdm_binary_SOURCES = 	\
	$(XDMCP_SOURCES)	\
	$(NULL)

CONSOLE_KIT_SOURCES = 		\
	$(NULL)

EXTRA_mdm_binary_SOURCES += $(CONSOLE_KIT_SOURCES)

# Note that these libs are in LDFLAGS because they should come before
# everything else on the link line as they may override stuff
mdm_binary_LDFLAGS =				\
	$(EXTRA_DAEMON_LIBS)			\
	$(NULL)

mdm_binary_LDADD = \
	$(top_builddir)/common/libmdmcommon.la	\
	$(XLIB_LIBS)				\
	$(DAEMON_LIBS)				\
	$(XDMCP_LIBS)                           \
	$(LIBWRAP_LIBS)                         \
	$(NULL)

if WITH_CONSOLE_KIT
mdm_binary_SOURCES += $(CONSOLE_KIT_SOURCES)
endif

sbin_SCRIPTS =		\
	mdm		\
	$(NULL)

mdm: $(srcdir)/mdm.in
	sed -e 's,[@]sbindir[@],$(sbindir),g' <$(srcdir)/mdm.in >mdm

CLEANFILES =			\
	mdm			\
	$(BUILT_SOURCES)	\
	$(NULL)

EXTRA_DIST = 				\
	mdm.in				\
	mdm-slave.xml			\
	mdm-simple-slave.xml		\
	mdm-factory-slave.xml		\
	mdm-product-slave.xml		\
	mdm-xdmcp-chooser-slave.xml	\
	mdm-session-direct.xml		\
	mdm-manager.xml			\
	mdm-display.xml			\
	mdm-xdmcp-greeter-display.xml	\
	mdm-xdmcp-chooser-display.xml	\
	mdm-static-display.xml		\
	mdm-transient-display.xml	\
	mdm-local-display-factory.xml	\
	mdm-static-factory-display.xml	\
	mdm-product-display.xml		\
	$(NULL)
