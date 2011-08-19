## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir) -I$(top_srcdir) \
	-DPACKAGE_LIBEXECDIR=\""$(libexecdir)"\" \
	-DPACKAGE_SYSCONFDIR=\""$(sysconfdir)"\" \
	-DPACKAGE_DATADIR=\""$(datadir)"\" \
	-DPACKAGE_BINDIR=\""$(bindir)"\" \
	-DPACKAGE_LOCALSTATEDIR=\""$(localstatedir)"\" \
	-DPACKAGE_LOCALEDIR=\""$(localedir)"\" \
	-DPACKAGE_LIBDIR=\""$(libdir)"\" \
	-D_POSIX_PTHREAD_SEMANTICS -D_REENTRANT	\
	-DPOLKIT_MATE_COMPILATION

lib_LTLIBRARIES=libpolkit-mate.la

libpolkit_mateincludedir=$(includedir)/PolicyKit/polkit-mate

libpolkit_mateinclude_HEADERS =              				\
	polkit-mate.h							\
	polkit-mate-context.h						\
	polkit-mate-action.h						\
	polkit-mate-toggle-action.h					\
	polkit-mate-auth.h

libpolkit_mate_la_SOURCES =                                		\
	polkit-mate-context.h		polkit-mate-context.c		\
	polkit-mate-action.h		polkit-mate-action.c		\
	polkit-mate-toggle-action.h	polkit-mate-toggle-action.c	\
	polkit-mate-auth.h		polkit-mate-auth.c

libpolkit_mate_la_CFLAGS = 	\
	$(GTK_CFLAGS)		\
	$(DBUS_GLIB_CFLAGS)	\
	$(POLKIT_DBUS_CFLAGS)	\
	$(POLKIT_GRANT_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

libpolkit_mate_la_LIBADD = 	\
	$(GTK_LIBS)		\
	$(DBUS_GLIB_LIBS)	\
	$(POLKIT_DBUS_LIBS)	\
	$(POLKIT_GRANT_LIBS)	\
	$(INTLLIBS)

libpolkit_mate_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)

clean-local :
	rm -f *~ $(BUILT_SOURCES)

