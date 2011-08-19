INCLUDES = \
	-DLOCALEDIR=\""$(datadir)/locale"\" \
	-DDATADIR=\""$(pkgdatadir)"\" 	\
	-DICONDIR="\"$(pkgdatadir)/icons\"" \
	$(DISABLE_DEPRECATED) \
	$(MATECONF_EDITOR_CFLAGS) \
	$(MATECONF_EDITOR_POLKIT_CFLAGS)

bin_PROGRAMS = mateconf-editor

mateconf_editor_built_headers = mateconf-marshal.h
mateconf_editor_built_cfiles = mateconf-marshal.c

mateconf_editor_SOURCES =			\
	mateconf-bookmarks.c		\
	mateconf-bookmarks.h		\
	mateconf-bookmarks-dialog.c	\
	mateconf-bookmarks-dialog.h	\
	mateconf-cell-renderer.c		\
	mateconf-cell-renderer.h 		\
	mateconf-editor-application.c	\
	mateconf-editor-application.h	\
	mateconf-editor-window.c		\
	mateconf-editor-window.h		\
	mateconf-key-editor.c		\
	mateconf-key-editor.h		\
	mateconf-list-model.c		\
	mateconf-list-model.h		\
	mateconf-policykit.c		\
	mateconf-policykit.h		\
	mateconf-stock-icons.c		\
	mateconf-stock-icons.h		\
	mateconf-tree-model.c		\
	mateconf-tree-model.h		\
	mateconf-util.c			\
	mateconf-util.h			\
	gedit-output-window.c		\
	gedit-output-window.h		\
	mateconf-search.h                  \
	mateconf-search.c                  \
	mateconf-search-dialog.h           \
	mateconf-search-dialog.c           \
	main.c				\
	$(mateconf_editor_built_headers)	\
	$(mateconf_editor_built_cfiles)

## we just punt on built sources, and check them into svn, manually updating
## when required.
regenerate-built-sources:
	echo "Building mateconf-marshal.h" \
	&& @GLIB_GENMARSHAL@ --prefix=mateconf_marshal mateconf-marshal.list --header > mateconf-marshal.h \
	&& echo "Building mateconf-marshal.c" \
	&& echo "#include <mateconf-marshal.h>" > mateconf-marshal.c \
        && @GLIB_GENMARSHAL@ --prefix=mateconf_marshal mateconf-marshal.list --body >> mateconf-marshal.c


mateconf_editor_LDADD = \
	$(MATECONF_EDITOR_LIBS) \
	$(MATECONF_EDITOR_POLKIT_LIBS)

EXTRA_DIST = \
	mateconf-marshal.list
