INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libmateui			\
	$(WARN_CFLAGS)					\
	$(LIBMATEUI_DEMOS_CFLAGS)			\
	-DG_LOG_DOMAIN=\"MateUI\"

LDADD =	\
	$(top_builddir)/libmateui/libmateui-2.la \
	$(LIBMATEUI_DEMOS_LIBS)

EXTRA_PROGRAMS = \
	mdi_demo		\
	animator_demo

animator_demo_SOURCES = animator_demo.c

mdi_demo_SOURCES = mdi_demo.c

