SUBDIRS = . tests
DIST_SUBDIRS = tests

lib_LTLIBRARIES = libtotem-plparser.la libtotem-plparser-mini.la

MARSHALFILES = totemplparser-marshal.c totemplparser-marshal.h
BUILT_SOURCES = $(MARSHALFILES)

totemplparser-marshal.c: totemplparser-marshal.list
	 $(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=totemplparser_marshal --header --body --internal $< > $@

totemplparser-marshal.h: totemplparser-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=totemplparser_marshal --header --internal $< > $@

plparserincludedir = $(pkgincludedir)/1/plparser
plparserinclude_HEADERS =			\
	totem-pl-parser-builtins.h		\
	totem-pl-parser-features.h		\
	totem-pl-parser.h			\
	totem-pl-playlist.h			\
	totem-disc.h				\
	totem-pl-parser-mini.h

plparser_sources =				\
	totem-pl-parser-builtins.c		\
	totem-disc.c				\
	totem-pl-parser.c			\
	totem-pl-parser-lines.c			\
	totem-pl-parser-media.c			\
	totem-pl-parser-misc.c			\
	totem-pl-parser-pla.c			\
	totem-pl-parser-pls.c			\
	totem-pl-parser-podcast.c		\
	totem-pl-parser-qt.c			\
	totem-pl-parser-smil.c			\
	totem-pl-parser-wm.c			\
	totem-pl-parser-xspf.c

nodist_libtotem_plparser_la_SOURCES = $(MARSHALFILES)
dist_libtotem_plparser_la_SOURCES =		\
	$(plparser_sources)			\
	totem-pl-parser-builtins.h		\
	totem-disc.h				\
	totem-pl-parser-features.h		\
	totem-pl-parser.h			\
	totem-pl-parser-lines.h			\
	totem-pl-parser-media.h			\
	totem-pl-parser-misc.h			\
	totem-pl-parser-pla.h			\
	totem-pl-parser-pls.h			\
	totem-pl-parser-podcast.h		\
	totem-pl-parser-private.h		\
	totem-pl-parser-qt.h			\
	totem-pl-parser-smil.h			\
	totem-pl-parser-wm.h			\
	totem-pl-parser-xspf.h			\
	totem-pl-playlist.c			\
	totem-pl-playlist.h			\
	xmlparser.h xmlparser.c			\
	xmllexer.h xmllexer.c			\
	bswap.h

libtotem_plparser_la_CPPFLAGS = \
	-I$(top_srcdir)				\
	-I$(top_srcdir)/lib			\
	-I$(top_builddir)/plparse		\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"\
	$(DISABLE_DEPRECATED)			\
	$(AM_CPPFLAGS)

libtotem_plparser_la_CFLAGS = \
	$(TOTEM_PLPARSER_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(WARN_CFLAGS)			\
	$(AM_CFLAGS)			\
	-D_FILE_OFFSET_BITS=64		\
	-D_LARGEFILE64_SOURCE		\
	-I$(top_srcdir)/lib		\
	-D_GNU_SOURCE

libtotem_plparser_la_LIBADD = \
	$(TOTEM_PLPARSER_LIBS)			\
	$(top_builddir)/lib/libtotem_glibc.la

libtotem_plparser_la_LDFLAGS =			\
	-version-info $(PLPARSER_LT_VERSION)	\
	-export-symbols $(srcdir)/plparser.symbols \
	-no-undefined				\
	$(AM_LDFLAGS)

libtotem_plparser_mini_la_SOURCES =		\
	totem-pl-parser-mini.h			\
	totem-pl-parser.c			\
	totem-pl-parser-lines.c			\
	totem-pl-parser-lines.h			\
	totem-pl-parser-misc.c			\
	totem-pl-parser-misc.h			\
	totem-pl-parser-pls.c			\
	totem-pl-parser-pls.h			\
	totem-pl-parser-podcast.c		\
	totem-pl-parser-podcast.h		\
	totem-pl-parser-private.h		\
	totem-pl-parser-qt.c			\
	totem-pl-parser-qt.h			\
	totem-pl-parser-smil.c			\
	totem-pl-parser-smil.h			\
	totem-pl-parser-wm.c			\
	totem-pl-parser-wm.h			\
	totem-pl-parser-xspf.c			\
	totem-pl-parser-xspf.h			\
	totem-pl-playlist.c			\
	totem-pl-playlist.h

libtotem_plparser_mini_la_CPPFLAGS = \
	-I$(top_srcdir)			\
	-I$(top_srcdir)/lib		\
	-I$(top_builddir)/plparse	\
	-DTOTEM_PL_PARSER_MINI		\
	$(DISABLE_DEPRECATED)		\
	$(AM_CPPFLAGS)

libtotem_plparser_mini_la_CFLAGS = \
	$(TOTEM_PLPARSER_MINI_CFLAGS)	\
	$(WARN_CFLAGS)			\
	$(AM_CFLAGS)

libtotem_plparser_mini_la_LIBADD =		\
	$(TOTEM_PLPARSER_MINI_LIBS)		\
	$(top_builddir)/lib/libtotem_glibc.la

libtotem_plparser_mini_la_LDFLAGS =			\
	-version-info $(PLPARSER_LT_VERSION)		\
	-export-symbols $(srcdir)/plparser-mini.symbols	\
	-no-undefined					\
	$(AM_LDFLAGS)

totem-pl-parser-builtins.h: stamp-totem-pl-parser-builtins.h
	@true

stamp-totem-pl-parser-builtins.h: totem-pl-parser.h totem-disc.h Makefile
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#ifndef __TOTEM_PL_PARSER_BUILTINS_H__\n#define __TOTEM_PL_PARSER_BUILTINS_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
			--fprod "/* enumerations from \"@filename@\" */\n" \
			--vhead "GType @enum_name@_get_type (void) G_GNUC_CONST;\n#define TOTEM_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" \
			--ftail "G_END_DECLS\n\n#endif /* __TOTEM_PL_PARSER_BUILTINS_H__ */" totem-pl-parser.h totem-disc.h) >> xgen-gtbh \
	&& (cmp -s xgen-gtbh totem-pl-parser-builtins.h || cp xgen-gtbh totem-pl-parser-builtins.h ) \
	&& rm -f xgen-gtbh \
	&& echo timestamp > $(@F)

totem-pl-parser-builtins.c: totem-pl-parser.h totem-disc.h Makefile totem-pl-parser-builtins.h
	$(AM_V_GEN) (cd $(srcdir) && $(GLIB_MKENUMS) \
			--fhead "#include \"totem-pl-parser.h\"\n#include \"totem-disc.h\"\n#include \"totem-pl-parser-builtins.h\"" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" \
			--vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
		totem-pl-parser.h totem-disc.h) > xgen-gtbc \
	&& cp xgen-gtbc totem-pl-parser-builtins.c  \
	&& rm -f xgen-gtbc

CLEANFILES = \
	totem-pl-parser-builtins.h \
	totem-pl-parser-builtins.c \
	stamp-totem-pl-parser-builtins.h \
	$(BUILT_SOURCES)

DISTCLEANFILES = $(CLEANFILES)

EXTRA_DIST =				\
	totemplparser-marshal.list	\
	totem-pl-parser-features.h.in	\
	plparser.symbols		\
	plparser-mini.symbols

# Introspection
-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --add-include-path=$(builddir)
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_sources = \
	$(plparser_sources)		\
	$(plparserinclude_HEADERS)	\
	totemplparser-marshal.c

TotemPlParser-1.0.gir: libtotem-plparser.la Makefile totemplparser-marshal.c totemplparser-marshal.h
TotemPlParser_1_0_gir_SCANNERFLAGS = --namespace=TotemPlParser --strip-prefix=TotemPl --nsversion=1.0
TotemPlParser_1_0_gir_INCLUDES = GObject-2.0 Gio-2.0
TotemPlParser_1_0_gir_CFLAGS = \
	$(TOTEM_PLPARSER_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(AM_CFLAGS)			\
	-D_FILE_OFFSET_BITS=64		\
	-D_LARGEFILE64_SOURCE		\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/lib		\
	-I$(top_builddir)/plparse	\
	-D_GNU_SOURCE
TotemPlParser_1_0_gir_LIBS = libtotem-plparser.la
TotemPlParser_1_0_gir_FILES = $(introspection_sources)
INTROSPECTION_GIRS += TotemPlParser-1.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
