lib_LTLIBRARIES = libmate-menu.la

AM_CPPFLAGS =				\
	$(GLIB_CFLAGS)			\
	-DMATEMENU_I_KNOW_THIS_IS_UNSTABLE	\
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(DEBUG_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

libmate_menu_includedir = $(includedir)/mate-menus
libmate_menu_include_HEADERS =		\
	matemenu-tree.h

libmate_menu_sources =			\
	canonicalize.c			\
	desktop-entries.c		\
	entry-directories.c		\
	matemenu-tree.c			\
	menu-layout.c			\
	menu-monitor.c			\
	menu-util.c

libmate_menu_la_SOURCES =		\
	$(libmate_menu_sources)	\
	canonicalize.h			\
	desktop-entries.h		\
	entry-directories.h		\
	matemenu-tree.h			\
	menu-layout.h			\
	menu-monitor.h			\
	menu-util.h

libmate_menu_la_LIBADD = 		\
	$(GLIB_LIBS)

libmate_menu_la_LDFLAGS =			\
	-version-info $(LIB_MENU_LT_VERSION)	\
	-no-undefined				\
	-export-symbols-regex matemenu_tree

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmate-menu.pc

EXTRA_DIST =				\
	libmate-menu.pc.in		\
	libmate-menu-uninstalled.pc.in

CLEANFILES =

# Introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
# Note: we only include the headers here so far because there's no gtk-doc at all anyway
introspection_sources = $(libmate_menu_include_HEADERS)

MateMenu-2.0.gir: libmate-menu.la
MateMenu_2_0_gir_INCLUDES = GObject-2.0
MateMenu_2_0_gir_CFLAGS = $(AM_CPPFLAGS)
MateMenu_2_0_gir_LIBS = libmate-menu.la
MateMenu_2_0_gir_SCANNERFLAGS = --pkg-export=libmate-menu
MateMenu_2_0_gir_FILES = $(addprefix $(srcdir)/,$(introspection_sources))
INTROSPECTION_GIRS += MateMenu-2.0.gir

girdir = $(INTROSPECTION_GIRDIR)
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(INTROSPECTION_TYPELIBDIR)
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

-include $(top_srcdir)/git.mk
