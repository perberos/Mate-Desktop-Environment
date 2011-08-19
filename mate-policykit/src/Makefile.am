
BUILT_SOURCES =							\
	polkit-mate-manager-glue.h

polkit-mate-manager-glue.h: polkit-mate-manager.xml Makefile.am
	dbus-binding-tool --prefix=polkit_mate_manager --mode=glib-server --output=polkit-mate-manager-glue.h polkit-mate-manager.xml

libexec_PROGRAMS = polkit-mate-manager

polkit_mate_manager_SOURCES = 						\
	polkit-mate-manager.h		polkit-mate-manager.c		\
	polkit-mate-auth-dialog.h	polkit-mate-auth-dialog.c	\
	sexy-url-label.h 		sexy-url-label.c		\
	main.c								\
	$(BUILT_SOURCES)

polkit_mate_manager_CPPFLAGS = 			\
	-I$(top_srcdir)					\
	-DG_LOG_DOMAIN=\"PolicyKitMate\"		\
	-DDATADIR=\""$(pkgdatadir)"\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

polkit_mate_manager_CFLAGS = 				\
	$(GTK_CFLAGS)					\
	$(MATECONF_CFLAGS)					\
	$(DBUS_GLIB_CFLAGS)				\
	$(POLKIT_DBUS_CFLAGS)				\
	$(POLKIT_GRANT_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)

polkit_mate_manager_LDFLAGS = 				\
	$(AM_LDFLAGS)

polkit_mate_manager_LDADD = 				\
	$(GTK_LIBS)					\
	$(MATECONF_LIBS)					\
	$(DBUS_GLIB_LIBS)				\
	$(POLKIT_DBUS_LIBS)				\
	$(POLKIT_GRANT_LIBS)				\
	$(INTLLIBS)

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = polkit-mate-manager.xml

clean-local :
	rm -f *~
