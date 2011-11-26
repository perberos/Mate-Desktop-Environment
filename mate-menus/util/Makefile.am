noinst_PROGRAMS = mate-menu-spec-test

AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	-I$(srcdir)/../libmenu \
	-DMATEMENU_I_KNOW_THIS_IS_UNSTABLE \
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(DEBUG_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

mate_menu_spec_test_SOURCES = \
	test-menu-spec.c

mate_menu_spec_test_LDADD = \
	$(GLIB_LIBS) \
	../libmenu/libmate-menu.la

if HAVE_PYTHON
pyexampledir = $(pkgdatadir)/examples
pyexample_DATA = mate-menus-ls.py
else
pyexampledir =
pyexample_DATA =
endif

EXTRA_DIST = $(pyexample_DATA)

-include $(top_srcdir)/git.mk

