INCLUDES = $(GST_TOOL_CFLAGS)				\
	$(CAJA_CFLAGS)				\
	-DDATADIR=\"$(datadir)\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(top_srcdir)					\
	-I$(top_builddir)

caja_extensiondir=$(CAJADIR)

caja_extension_LTLIBRARIES=libcaja-gst-shares.la

libcaja_gst_shares_la_SOURCES =     		\
	caja-shares.c	caja-shares.h	\
	gst-shares-module.c

libcaja_gst_shares_la_LDFLAGS = -module -avoid-version
libcaja_gst_shares_la_LIBADD  = $(CAJA_LIBS)


-include $(top_srcdir)/git.mk
