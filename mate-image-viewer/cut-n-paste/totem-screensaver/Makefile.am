noinst_LTLIBRARIES = libtotemscrsaver.la

libtotemscrsaver_la_SOURCES = \
	totem-scrsaver.h \
	totem-scrsaver.c

libtotemscrsaver_la_LIBADD = $(X11_LIBS)

libtotemscrsaver_la_CFLAGS = \
	-I$(top_srcdir)/src	\
	$(EOG_CFLAGS)		\
	$(X11_CFLAGS)		\
	$(WARN_CFLAGS)

-include $(top_srcdir)/git.mk
