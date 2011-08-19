noinst_LTLIBRARIES =					\
	libtotem_glibc.la

common_defines = \
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATADIR=\""$(datadir)"\"			\
	$(DISABLE_DEPRECATED)

noinst_HEADERS = totem_internal.h

libtotem_glibc_la_SOURCES = stub.c
libtotem_glibc_la_LIBADD = @LTLIBOBJS@

EXTRA_DIST = asprintf.c

