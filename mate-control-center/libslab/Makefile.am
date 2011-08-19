INCLUDES =					\
	-I$(top_srcdir)				\
	$(LIBSLAB_CFLAGS)			\
	$(WARN_CFLAGS)


HEADER_FILES=					\
	$(BUILT_SOURCES)			\
	app-resizer.h				\
	app-shell.h				\
	application-tile.h			\
	bookmark-agent.h			\
	directory-tile.h			\
	document-tile.h				\
	double-click-detector.h			\
	mate-utils.h				\
	libslab-utils.h				\
	nameplate-tile.h			\
	search-bar.h				\
	search-context-picker.h			\
	search-entry.h				\
	shell-window.h				\
	slab-mate-util.h			\
	slab-section.h				\
	slab.h					\
	system-tile.h				\
	tile.h

if !LIBSLAB_FOR_INTERNAL_USE
lib_LTLIBRARIES = libslab.la
else
noinst_LTLIBRARIES = libslab.la
endif

libslab_la_SOURCES =				\
	$(MARSHAL_GENERATED)			\
	app-resizer.c				\
	app-shell.c				\
	application-tile.c			\
	bookmark-agent.c			\
	directory-tile.c			\
	document-tile.c				\
	double-click-detector.c			\
	mate-utils.c				\
	libslab-utils.c				\
	nameplate-tile.c			\
	search-bar.c				\
	search-context-picker.c			\
	search-entry-watermark.h		\
	search-entry.c				\
	shell-window.c				\
	slab-mate-util.c			\
	slab-section.c				\
	system-tile.c				\
	themed-icon.c				\
	themed-icon.h				\
	tile-action.c				\
	tile.c

if !LIBSLAB_FOR_INTERNAL_USE
libslab_includedir = $(includedir)/libslab
libslab_include_HEADERS = $(HEADER_FILES)

libslab_la_LDFLAGS = -version-info $(LT_VERSION)
endif

libslab_la_LIBADD = $(LIBSLAB_LIBS)

search-entry-watermark.h: search-entry-watermark.svg
	echo '#define SEARCH_ENTRY_WATERMARK_SVG "\' > $@; \
	sed -e 's/"/\\"/g' -e 's/$$/\\/' -e 's/#000000/#%s/g' $< >> $@; \
	echo '"' >> $@


MARSHAL_GENERATED = nld-marshal.c nld-marshal.h

nld-marshal.h: nld-marshal.list
	( @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --header > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.h ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )

nld-marshal.c: nld-marshal.h
	( (echo '#include "nld-marshal.h"'; @GLIB_GENMARSHAL@ --prefix=nld_marshal $(srcdir)/nld-marshal.list --body) > nld-marshal.tmp \
	&& mv nld-marshal.tmp nld-marshal.c ) \
	|| ( rm -f nld-marshal.tmp && exit 1 )


BUILT_SOURCES =				\
	search-entry-watermark.h	\
	$(MARSHAL_GENERATED)

CLEANFILES =				\
	search-entry-watermark.h	\
	$(MARSHAL_GENERATED)

EXTRA_DIST=				\
	search-entry-watermark.svg	\
	nld-marshal.list

-include $(top_srcdir)/git.mk
