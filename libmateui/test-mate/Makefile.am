## Process this file with automake to produce Makefile.in

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/libmateui			\
	$(WARN_CFLAGS)					\
	$(LIBMATEUI_CFLAGS)				\
	-DMATEUIPIXMAPDIR=\""$(datadir)/pixmaps"\"	\
	-DMATEUISRCDIR=\""$(srcdir)"\"			\
	-DG_LOG_DOMAIN=\"MateUI\"

LDADD = \
	$(top_builddir)/libmateui/libmateui-2.la $(MATE_TEST_LIBS)

noinst_PROGRAMS = \
	test-mate test-druid test-entry test-iconlist test-password-dialog test-mategdkpixbuf

test_mate_SOURCES =		\
	testmate.c		\
	testmate.h

test_druid_SOURCES =	\
	testdruid.c

test_password_dialog_SOURCES =	\
	prop-editor.c \
	prop-editor.h \
	test-password-dialog.c

test_entry_SOURCES = 	\
	testentry.c

test_iconlist_SOURCES = 	\
	testiconlist.c

test_mategdkpixbuf_SOURCES = 	\
	testmategdkpixbuf.c

EXTRA_DIST = 		\
	bomb.xpm	\
	testmate.xml

# the program has to be run from $srcdir to find its image files
test: test-mate
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute $$blddir/test-mate

test-debug: test-mate
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute gdb $$blddir/test-mate

