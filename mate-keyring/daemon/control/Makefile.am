if WITH_TESTS
TESTS_DIR = tests
else
TESTS_DIR =
endif

SUBDIRS = . $(TESTS_DIR)

INCLUDES= \
	-DPREFIX=\""$(prefix)"\" \
	-DBINDIR=\""$(bindir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/daemon \
	-I$(top_builddir) \
	$(GLIB_CFLAGS)

# ------------------------------------------------------------------
# DAEMON CODE

noinst_LTLIBRARIES = \
	libgkd-control.la \
	libgkd-control-client.la

libgkd_control_la_SOURCES = \
	gkd-control-server.c gkd-control-client.c \
	gkd-control.h gkd-control-codes.h

libgkd_control_client_la_SOURCES = \
	gkd-control-client.c gkd-control.h
