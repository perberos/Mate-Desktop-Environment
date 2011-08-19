themedir = $(datadir)/icons/HighContrastLargePrint

SUBDIRS = 36x36 48x48 

THEME_IN_FILES = index.theme.in

%.theme:   %.theme.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po) ; $(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -d -u -c $(top_builddir)/po/.intltool-merge-cache

theme_DATA = index.theme

EXTRA_DIST=				\
	index.theme.in

CLEANFILES = 				\
	index.theme


MAINTAINERCLEANFILES =			\
	Makefile.in
