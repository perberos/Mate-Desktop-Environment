libmatekeyringincludedir = $(includedir)/mate-keyring-1/

lib_LTLIBRARIES=libmate-keyring.la

INCLUDES= \
	-DPREFIX=\""$(prefix)"\" \
	-DBINDIR=\""$(bindir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(LIBRARY_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(LIBGCRYPT_CFLAGS)

libmate_keyring_la_SOURCES = \
	gkr-callback.c gkr-callback.h \
	gkr-misc.c gkr-misc.h \
	gkr-operation.c gkr-operation.h \
	gkr-session.c gkr-session.h \
	mate-keyring.c \
	mate-keyring.h \
	mate-keyring-memory.h \
	mate-keyring-memory.c \
	mate-keyring-private.h \
	mate-keyring-result.h \
	mate-keyring-utils.c

libmatekeyringinclude_HEADERS = \
	mate-keyring.h \
	mate-keyring-memory.h \
	mate-keyring-result.h

libmate_keyring_la_LIBADD = \
	$(top_builddir)/egg/libegg.la \
	$(LIBRARY_LIBS) \
	$(GLIB_LIBS) \
	$(LIBGCRYPT_LIBS)

libmate_keyring_la_LDFLAGS = \
	-version-info $(LIB_MATE_KEYRING_LT_VERSION) \
	-no-undefined -export-symbols-regex 'mate_keyring_|MATE_KEYRING_'

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-keyring-1.pc

EXTRA_DIST = \
	mate-keyring-1-uninstalled.pc.in

if WITH_TESTS
TESTS_DIR = tests
else
TESTS_DIR =
endif

SUBDIRS = . \
	$(TESTS_DIR)
