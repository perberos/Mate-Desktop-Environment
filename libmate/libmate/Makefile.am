## Process this file with automake to produce Makefile.in

#
# Targets
#

lib_LTLIBRARIES = libmate-2.la

bin_PROGRAMS = mate-open

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libmateincludedir = $(includedir)/libmate-2.0/libmate

INCLUDES = \
	-I..							\
	-I$(srcdir)/..						\
	$(WARN_CFLAGS)						\
	$(LIBMATE_CFLAGS)					\
	-DG_DISABLE_DEPRECATED					\
	-DLIBMATE_PREFIX=\""$(prefix)"\"			\
	-DLIBMATE_LIBDIR=\""$(libdir)"\"			\
	-DLIBMATE_DATADIR=\""$(datadir)"\"			\
	-DLIBMATE_LOCALSTATEDIR=\""$(localstatedir)"\"		\
	-DLIBMATE_SYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBMATE_LOCALEDIR="\"$(datadir)/locale\""		\
	-DMATEVFSVERSION=\"$(MATE_VFS_VERSION)\"		\
	-DMATE_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE"	\
	-DG_LOG_DOMAIN=\"Mate\"

if OS_WIN32
platform_sources = dllmain.c
endif

libmate_2_la_SOURCES = \
	libmatetypebuiltins.h	\
	libmatetypebuiltins.c	\
	mate-config.c		\
	mate-sound.c		\
	mate-triggers.c	\
	mate-triggersP.h	\
	mate-program.c		\
	mate-help.c		\
	mate-exec.c		\
	mate-mateconf.c		\
	mate-mateconfP.h		\
	mate-i18n.c		\
	mate-url.c		\
	mate-util.c		\
	mate-score.c		\
	mate-init.c		\
	$(platform_sources)

noinst_HEADERS = \
	libmate-private.h

libmate_headers = \
	mate-config.h		\
	mate-sound.h		\
	mate-triggers.h	\
	mate-program.h		\
	mate-help.h		\
	mate-i18n.h		\
	mate-exec.h		\
	mate-mateconf.h		\
	mate-url.h		\
	mate-util.h   		\
	mate-macros.h		\
	mate-score.h		\
	mate-init.h		\
	libmate.h

libmateinclude_HEADERS = \
	libmatetypebuiltins.h	\
	$(libmate_headers)

mate_open_SOURCES = \
	mate-open.c

mate_open_LDADD = \
	libmate-2.la	\
	$(LIBMATE_LIBS) \
	$(GIO_LIBS)

libmatetypebuiltins.h: stamp-libmatetypebuiltins.h
	$(AM_V_GEN) true
stamp-libmatetypebuiltins.h: Makefile @REBUILD@ $(libmate_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#ifndef __LIBMATETYPEBUILTINS_H__\n" \
			--fhead "#define __LIBMATETYPEBUILTINS_H__ 1\n\n" \
			--fhead "#include <libmate/libmate.h>\n\n" \
			--fhead "G_BEGIN_DECLS\n\n" \
			--ftail "G_END_DECLS\n\n" \
			--ftail "#endif /* __LIBMATETYPEBUILTINS_H__ */\n" \
			--fprod "\n/* --- @filename@ --- */" \
			--eprod "#define MATE_TYPE_@ENUMSHORT@ @enum_name@_get_type()\n" \
			--eprod "GType @enum_name@_get_type (void);\n" \
		$(libmate_headers) ) > xgen-gth \
	&& (cmp -s xgen-gth libmatetypebuiltins.h || cp xgen-gth libmatetypebuiltins.h) \
	&& rm -f xgen-gth \
	&& echo timestamp > $(@F)

libmatetypebuiltins.c: Makefile @REBUILD@ $(libmate_headers)
	$(AM_V_GEN) (cd $(srcdir) \
	&& glib-mkenums \
			--fhead "#include <config.h>\n" \
			--fhead "#include <glib-object.h>\n" \
			--fhead "#include \"libmatetypebuiltins.h\"\n\n" \
			--fprod "\n/* enumerations from \"@filename@\" */" \
			--vhead "static const G@Type@Value _@enum_name@_values[] = {" \
			--vprod "  { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
			--vtail "  { 0, NULL, NULL }\n};\n\n" \
			--vtail "GType\n@enum_name@_get_type (void)\n{\n" \
			--vtail "  static GType type = 0;\n\n" \
			--vtail "  if (!type)\n" \
			--vtail "    type = g_@type@_register_static (\"@EnumName@\", _@enum_name@_values);\n\n" \
			--vtail "  return type;\n}\n\n" \
		$(libmate_headers) ) > xgen-gtc \
	&& cp xgen-gtc $(@F) \
	&& rm -f xgen-gtc

BUILT_SOURCES = libmatetypebuiltins.h libmatetypebuiltins.c

MAINTAINERCLEANFILES = stamp-libmatetypebuiltins.h \
  $(BUILT_SOURCES)
CLEANFILES = xgen-gth xgen-gtc

# if srcdir!=builddir, clean out maintainer-clean files from builddir
# this allows dist to pass.
distclean-local:
	if test $(srcdir) != .; then \
	  rm -f $(MAINTAINERCLEANFILES); \
	fi

EXTRA_DIST = \
	libmate-2.0.pc.in		\
	libmate-2.0-uninstalled.pc.in	\
	parse-path.cP

libmate_2_la_LDFLAGS = \
	-version-info $(LIBMATE_CURRENT):$(LIBMATE_REVISION):$(LIBMATE_AGE) \
	-no-undefined \
	-export-symbols-regex "^[^_].*"

libmate_2_la_LIBADD =		\
	$(LIBMATE_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmate-2.0.pc

