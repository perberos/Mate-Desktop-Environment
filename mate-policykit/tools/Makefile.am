
bin_PROGRAMS = polkit-mate-authorization

polkit_mate_authorization_SOURCES = 						\
						polkit-mate-authorization.c	\
	sexy-url-label.h 			sexy-url-label.c

polkit_mate_authorization_CPPFLAGS = 			\
	-I$(top_srcdir)					\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DDATADIR=\""$(pkgdatadir)"\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

polkit_mate_authorization_CFLAGS = 			\
	$(GTK_CFLAGS)					\
	$(MATECONF_CFLAGS)					\
	$(DBUS_GLIB_CFLAGS)				\
	$(POLKIT_DBUS_CFLAGS)				\
	$(POLKIT_GRANT_CFLAGS)				\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)

polkit_mate_authorization_LDFLAGS = 			\
	$(AM_LDFLAGS)

polkit_mate_authorization_LDADD = 			\
	$(GTK_LIBS)					\
	$(MATECONF_LIBS)					\
	$(DBUS_GLIB_LIBS)				\
	$(POLKIT_DBUS_LIBS)				\
	$(POLKIT_GRANT_LIBS)				\
	$(INTLLIBS)					\
	$(top_builddir)/polkit-mate/libpolkit-mate.la

BUILT_SOURCES =						\
	single-instance-glue.h

single-instance-glue.h: single-instance.xml Makefile.am
	dbus-binding-tool --prefix=single_instance --mode=glib-server --output=single-instance-glue.h $(top_srcdir)/tools/single-instance.xml

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = single-instance.xml

clean-local :
	rm -f *~
