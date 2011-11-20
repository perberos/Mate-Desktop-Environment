AM_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(PYTHON_INCLUDES) \
	-I$(srcdir)/../libmenu \
	-DMATEMENU_I_KNOW_THIS_IS_UNSTABLE \
	$(DISABLE_DEPRECATED_CFLAGS) \
	$(DEBUG_CFLAGS)

AM_CFLAGS = \
	$(WARN_CFLAGS) \
	$(NO_STRICT_ALIASING_CFLAGS)

matemenudir = $(pyexecdir)
matemenu_LTLIBRARIES = matemenu.la

matemenu_la_SOURCES = matemenu.c
matemenu_la_LDFLAGS = -module -avoid-version -fPIC -export-symbols-regex initmatemenu
matemenu_la_LIBADD = $(GLIB_LIBS) $(PYTHON_LIBS) $(top_builddir)/libmenu/libmate-menu.la

-include $(top_srcdir)/git.mk
