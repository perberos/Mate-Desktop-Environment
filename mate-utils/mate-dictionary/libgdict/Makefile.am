NULL =

INCLUDES = -DG_LOG_DOMAIN=\"Gdict\" 				\
	-DDATADIR=\""$(datadir)"\" 				\
	-DLIBDIR=\""$(libdir)"\" 				\
	-DSYSCONFDIR=\""$(sysconfdir)"\" 			\
	-DPREFIX=\""$(prefix)"\" 				\
	-DMATELOCALEDIR=\""$(mateutilslocaledir)"\" 		\
	-DGDICTSOURCESDIR=\""$(datadir)/gdict-1.0/sources"\" 	\
	-DGDICT_ENABLE_INTERNALS=1 				\
	-DG_DISABLE_DEPRECATED					\
	-DGDK_DISABLE_DEPRECATED				\
	-DGTK_DISABLE_DEPRECATED				\
	-DPANGO_DISABLE_DEPRECATED				\
	-DG_DISABLE_SINGLE_INCLUDES				\
	-DGTK_DISABLE_SINGLE_INCLUDES				\
	$(NULL)

sources_h =	\
	$(srcdir)/gdict-context.h		\
	$(srcdir)/gdict-client-context.h	\
	$(srcdir)/gdict-database-chooser.h 	\
	$(srcdir)/gdict-defbox.h 		\
	$(srcdir)/gdict-source-chooser.h	\
	$(srcdir)/gdict-source-loader.h		\
	$(srcdir)/gdict-source.h		\
	$(srcdir)/gdict-speller.h		\
	$(srcdir)/gdict-strategy-chooser.h 	\
	$(srcdir)/gdict-utils.h			\
	$(NULL)

sources_h_priv =	\
	$(srcdir)/gdict-context-private.h	\
	$(srcdir)/gdict-debug.h			\
	$(srcdir)/gdict-private.h		\
	$(NULL)

sources_c =	\
	$(srcdir)/gdict-context.c		\
	$(srcdir)/gdict-client-context.c	\
	$(srcdir)/gdict-database-chooser.c	\
	$(srcdir)/gdict-defbox.c		\
	gdict-enum-types.c			\
	gdict-marshal.c				\
	$(srcdir)/gdict-source-chooser.c	\
	$(srcdir)/gdict-source-loader.c		\
	$(srcdir)/gdict-source.c		\
	$(srcdir)/gdict-speller.c		\
	$(srcdir)/gdict-strategy-chooser.c	\
	$(srcdir)/gdict-utils.c			\
	$(NULL)

lib_LTLIBRARIES = libgdict-1.0.la

# MATE Dictionary shared library
libgdict_1_0_la_SOURCES  = $(sources_h) $(sources_h_priv) $(sources_c)
libgdict_1_0_la_CPPFLAGS = $(LIBGDICT_CFLAGS) $(GDICT_DEBUG_CFLAGS) $(MAINTAINER_CFLAGS)
libgdict_1_0_la_LIBADD   = $(LIBGDICT_LIBS)
libgdict_1_0_la_LDFLAGS  = \
	-version-info $(LIBGDICT_LT_VERSION) \
	-export-dynamic \
	-no-undefined

libgdict_includedir = $(includedir)/gdict-1.0/gdict
libgdict_include_HEADERS = 	\
	$(sources_h)		\
	gdict-enum-types.h	\
	gdict-version.h		\
	$(srcdir)/gdict.h	\
	$(NULL)

gdict_built_files = 		\
	gdict-enum-types.h	\
	gdict-enum-types.c	\
	gdict-marshal.h		\
	gdict-marshal.c

stamp_files = stamp-gdict-enum-types.h stamp-gdict-marshal.h

CLEANFILES = $(stamp_files) $(gdict_built_files)
DISTCLEANFILES = gdict-version.h
MAINTAINERCLEANFILES = $(stamp_files) $(gdict_built_files) gdict-version.h

EXTRA_DIST = 			\
	gdict-version.h.in 	\
	gdict-enum-types.h.in	\
	gdict-enum-types.c.in	\
	gdict-marshal.list	\
	gdict-1.0.pc.in		\
	$(NULL)

BUILT_SOURCES = $(gdict_built_files) 

gdict-marshal.h: stamp-gdict-marshal.h
	@true
stamp-gdict-marshal.h: gdict-marshal.list $(GLIB_GENMARSHAL) Makefile
	$(AM_V_GEN)$(GLIB_GENMARSHAL) $< --header --prefix=gdict_marshal > xgen-gmh \
	&& ( cmp -s xgen-gmh gdict-marshal.h || cp xgen-gmh gdict-marshal.h ) \
	&& rm -f xgen-gmh \
	&& echo timestamp > $(@F)

gdict-marshal.c: gdict-marshal.list $(GLIB_GENMARSHAL) Makefile
	$(AM_V_GEN)( echo "#include \"gdict-marshal.h\""; echo; \
	  $(GLIB_GENMARSHAL) $< --body --prefix=gdict_marshal ) > xgen-gmc \
	&& cp xgen-gmc gdict-marshal.c \
	&& rm -f xgen-gmc

gdict-enum-types.h: stamp-gdict-enum-types.h
	@true
stamp-gdict-enum-types.h: $(sources_h) gdict-enum-types.h.in Makefile
	$(AM_V_GEN)( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/gdict-enum-types.h.in \
	  $(sources_h) ) > xgen-ceth \
	&& ( cmp -s xgen-ceth gdict-enum-types.h || cp xgen-ceth gdict-enum-types.h ) \
	&& rm -f xgen-ceth \
	&& echo timestamp > $(@F)

gdict-enum-types.c: gdict-enum-types.h gdict-enum-types.c.in Makefile
	$(AM_V_GEN)( cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	  	--template $(srcdir)/gdict-enum-types.c.in \
	  $(sources_h) ) > xgen-cetc \
	&& cp xgen-cetc gdict-enum-types.c \
	&& rm -f xgen-cetc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = gdict-1.0.pc

distclean-local:
	if test $(srcdir) != .; then \
		rm -f $(MAINTAINERCLEANFILES); \
	fi

