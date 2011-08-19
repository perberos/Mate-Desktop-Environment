NULL =

INCLUDES =						\
	-DDATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
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

libexec_PROGRAMS = gdu-format-tool

gdu_format_tool_SOURCES =						\
					gdu-format-tool.c		\
	gdu-format-progress-dialog.h	gdu-format-progress-dialog.c	\
	$(NULL)

gdu_format_tool_CPPFLAGS = $(CORE_CFLAGS) -DG_LOG_DOMAIN=\"GDU-Format-Tool\"
gdu_format_tool_LDFLAGS = $(AM_LDFLAGS)
gdu_format_tool_LDADD = $(CORE_LIBADD)

EXTRA_DIST = TODO
