NULL =

INCLUDES =						\
	-DDATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\"$(libexecdir)\"			\
	-I$(top_srcdir)/src				\
	-I$(top_builddir)/src				\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)					\
	-DGDU_API_IS_SUBJECT_TO_CHANGE			\
	-DGDU_GTK_API_IS_SUBJECT_TO_CHANGE		\
	$(NULL)

CORE_CFLAGS = 						\
	$(GLIB2_CFLAGS)					\
	$(GOBJECT2_CFLAGS)				\
	$(GIO2_CFLAGS)					\
	$(GIO_UNIX2_CFLAGS)				\
	$(GTHREAD2_CFLAGS)				\
	$(GTK2_CFLAGS)					\
	$(NULL)

CORE_LIBADD = 						\
	$(GLIB2_LIBS)					\
	$(GOBJECT2_LIBS)				\
	$(GIO2_LIBS)					\
	$(GIO_UNIX2_LIBS)				\
	$(GTHREAD2_LIBS)				\
	$(GTK2_LIBS)					\
	$(INTLLIBS)					\
	$(top_builddir)/src/gdu/libgdu.la		\
	$(top_builddir)/src/gdu-gtk/libgdu-gtk.la	\
	$(NULL)


caja_extensiondir = $(libdir)/caja/extensions-2.0
caja_extension_LTLIBRARIES = libcaja-gdu.la

libcaja_gdu_la_SOURCES =			\
	caja-module.c			\
	caja-gdu.c				\
	caja-gdu.h				\
	$(NULL)
libcaja_gdu_la_CPPFLAGS = $(CORE_CFLAGS) $(CAJA_CFLAGS) -DG_LOG_DOMAIN=\"Caja-GDU\"
libcaja_gdu_la_LDFLAGS = $(AM_LDFLAGS) -export_dynamic -module -avoid-version -no-undefined
libcaja_gdu_la_LIBADD  = $(CORE_LIBADD) $(CAJA_LIBS)
