AM_CPPFLAGS =				\
	$(GLIB_CFLAGS)			\
	$(PYTHON_INCLUDES)		\
	-I$(srcdir)/../libmenu		\
	-DGMENU_I_KNOW_THIS_IS_UNSTABLE	\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(DEBUG_CFLAGS)

AM_CFLAGS =
	$(WARN_CFLAGS)			\
	$(NO_STRICT_ALIASING_CFLAGS)

gmenudir = $(pyexecdir)
gmenu_LTLIBRARIES = gmenu.la

gmenu_la_SOURCES = gmenu.c
gmenu_la_LDFLAGS = -module -avoid-version -fPIC -export-symbols-regex initgmenu
gmenu_la_LIBADD = $(GLIB_LIBS) $(PYTHON_LIBS) $(top_builddir)/libmenu/libmate-menu.la

-include $(top_srcdir)/git.mk
