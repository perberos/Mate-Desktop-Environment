## Process this file with automake to produce Makefile.in

SUBDIRS = pixmaps

#
# The targets
#
lib_LTLIBRARIES = libmateui-2.la

matehdir = $(includedir)/libmateui-2.0
mateh_HEADERS = mate.h

#
# Install location for header files
#
libmateuiincludedir = $(includedir)/libmateui-2.0/libmateui

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-DLIBMATEUI_PREFIX=\""$(prefix)"\"		\
        -DLIBMATEUI_DATADIR=\""$(datadir)"\" 		\
        -DLIBMATEUI_SERVERDIR=\""$(libexecdir)"\" 	\
	-DG_LOG_DOMAIN=\"MateUI\"			\
	-DMATEUILOCALEDIR="\"$(datadir)/locale\""	\
	-DMATE_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE"	\
	$(WARN_CFLAGS)					\
	$(LIBMATEUI_CFLAGS)

EXTRA_HEADERS = 

if OS_WIN32
platform_dep_sources = \
	mate-ui-win32.c
else
platform_dep_sources = \
	mate-authentication-manager.c  \
	mate-authentication-manager.h  \
	mate-authentication-manager-private.h
endif


libmateui_2_la_SOURCES = \
	libmateui-access.c		\
	libmateui-access.h		\
	matetypebuiltins.h		\
	matetypebuiltins.c		\
	mate-about.c			\
	mate-app.c			\
	mate-app-helper.c		\
	mate-app-util.c		\
	mate-appbar.c			\
	mate-stock-icons.c		\
	mate-client.c			\
	mate-color-picker.c		\
	mate-dateedit.c		\
	mate-dialog.c			\
	mate-dialog-util.c		\
	mate-druid.c			\
	mate-druid-page.c		\
	mate-druid-page-edge.c		\
	mate-druid-page-standard.c	\
	mate-entry.c			\
	mate-file-entry.c		\
	mate-font-picker.c		\
	mate-mateconf-ui.c		\
	mate-href.c			\
	mate-ice.c			\
	mate-marshal-main.c		\
	mate-messagebox.c		\
	mate-mdi.c			\
	mate-mdi-child.c		\
	mate-mdi-generic-child.c	\
	mate-mdi-session.c		\
	mate-multiscreen.c		\
	mate-pixmap.c			\
	mate-pixmap-entry.c		\
	mate-popup-menu.c		\
	mate-propertybox.c		\
	mate-scores.c			\
	mate-theme-parser.c		\
	mate-thumbnail.c		\
	mate-thumbnail-pixbuf-utils.c	\
	mate-ui-init.c			\
	matetypes.c			\
	mate-icon-entry.c		\
	mate-icon-item.c		\
	mate-icon-list.c		\
	mate-icon-lookup.c		\
	mate-icon-sel.c		\
	mate-icon-theme.c		\
	mate-vfs-util.c		\
	mate-window.c			\
	mate-window-icon.c		\
	mate-mateconf-ui.h		\
	libmateuiP.h			\
	mate-password-dialog.c         \
	mate-password-dialog.h         \
	$(platform_dep_sources)

## this lists all the non-generated headers
mate_headers = \
	mate-about.h			\
	mate-app.h			\
	mate-app-helper.h		\
	mate-app-util.h		\
	mate-appbar.h			\
	mate-authentication-manager.h  \
	mate-client.h			\
	mate-color-picker.h		\
	mate-dateedit.h		\
	mate-dialog.h			\
	mate-dialog-util.h		\
	mate-druid.h			\
	mate-druid-page.h		\
	mate-druid-page-edge.h		\
	mate-druid-page-standard.h	\
	mate-entry.h			\
	mate-file-entry.h		\
	mate-font-picker.h		\
	mate-help.h			\
	mate-href.h			\
	mate-ice.h			\
	mate-icon-entry.h		\
	mate-icon-item.h		\
	mate-icon-list.h		\
	mate-icon-lookup.h		\
	mate-icon-sel.h		\
	mate-icon-theme.h		\
	mate-messagebox.h		\
	mate-mdi.h			\
	mate-mdi-child.h		\
	mate-mdi-generic-child.h	\
	mate-mdi-session.h		\
	mate-password-dialog.h         \
	mate-pixmap.h			\
	mate-pixmap-entry.h		\
	mate-popup-menu.h		\
	mate-propertybox.h		\
	mate-scores.h			\
	mate-ui-init.h			\
	mate-stock-icons.h		\
	mate-thumbnail.h		\
	mate-types.h			\
	mate-uidefs.h			\
	mate-url.h			\
	mate-vfs-util.h		\
	mate-window.h			\
	mate-window-icon.h		\
	libmateui.h

mate_private_headers = \
	mate-theme-parser.h


libmateuiinclude_HEADERS =		\
	$(mate_headers)		\
	$(mate_private_headers)	\
	matetypebuiltins.h

BUILT_SOURCES = matetypebuiltins.h matetypebuiltins.c matetype_inits.c \
  mate-marshal.h mate-marshal.c

matetypebuiltins.h: stamp-matetypebuiltins.h
	@true
stamp-matetypebuiltins.h: @REBUILD@ $(mate_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __MATETYPEBUILTINS_H__\n" \
			--fhead "#define __MATETYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <glib-object.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __MATETYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define MATE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(mate_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth matetypebuiltins.h || cp xgen-gth matetypebuiltins.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

matetypebuiltins.c: @REBUILD@ $(mate_headers) matetypebuiltins.h
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include \"libmateui.h\"\n\n" \
			--fhead "#include \"matetypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n" \
			--vhead "  static GType type = 0;\n\n" \
			--vhead "  if (G_UNLIKELY (type == 0))\n  {\n" \
			--vhead "    static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n  }\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(mate_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc

matetype_inits.c: @REBUILD@ $(mate_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vtail "    (void) @enum_name@_get_type ();\n" \
		$(mate_headers) ) > xgen-gtic \
	&& cp xgen-gtic $(@F) \
	&& rm -f xgen-gtic

mate-marshal.h: stamp-mate-marshal.h
	$(AM_V_GEN) true
stamp-mate-marshal.h: @REBUILD@ mate-marshal.list
	$(AM_V_GEN) glib-genmarshal --prefix=_mate_marshal $(srcdir)/mate-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh mate-marshal.h || cp xgen-gmh mate-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~ \
	&& echo timestamp > $(@F)

mate-marshal.c: @REBUILD@ mate-marshal.list
	$(AM_V_GEN) glib-genmarshal --prefix=_mate_marshal $(srcdir)/mate-marshal.list --body >> xgen-gmc \
	&& cp xgen-gmc mate-marshal.c \
	&& rm -f xgen-gmc xgen-gmc~

MAINTAINERCLEANFILES = stamp-matetypebuiltins.h stamp-mate-marshal.h \
  $(BUILT_SOURCES)
CLEANFILES = xgen-gmc xgen-gmh xgen-gth xgen-gtc xgen-gtic

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

#######################

EXTRA_DIST =					\
	AUTHORS					\
	mate-marshal.list			\
	mate-boxed.defs			\
	matetype_inits.c			\
	libmateui-2.0.pc.in			\
	$(BUILT_SOURCES)

libmateui_2_la_LDFLAGS = \
	-version-info $(LIBMATEUI_CURRENT):$(LIBMATEUI_REVISION):$(LIBMATEUI_AGE) \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

libmateui_2_la_LIBADD = \
	$(LIBMATEUI_LIBS)			\
	$(SM_LIBS)				\
	$(X11_LIBS)

LDADD = \
	libmateui-2.la $(LIBMATEUI_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmateui-2.0.pc

