
INCLUDES= \
	-DPREFIX=\""$(prefix)"\" \
	-DBINDIR=\""$(bindir)"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/daemon \
	-I$(top_builddir) \
	$(GOBJECT_CFLAGS) \
	$(GLIB_CFLAGS)

# ------------------------------------------------------------------
# DAEMON CODE

noinst_LTLIBRARIES = libgkd-login.la

libgkd_login_la_SOURCES = \
	gkd-login.c gkd-login.h

libgkd_login_la_LIBADD = \
	$(top_builddir)/gp11/libmategp11.la \
	$(GOBJECT_LIBS) \
	$(GLIB_LIBS)
