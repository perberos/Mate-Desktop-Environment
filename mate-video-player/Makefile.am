NORMAL_SUBDIRS = lib bindings po data help src docs
SUBDIRS = $(NORMAL_SUBDIRS)

if ENABLE_BROWSER_PLUGINS
SUBDIRS += browser-plugin
endif


EXTRA_DIST =			\
	license_change		\
	autogen.sh		\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	AUTHORS NEWS		\
	totem.spec.in		\
	totem.spec		\
	omf.make		\
	xmldocs.make		\
	mate-doc-utils.make

CLEANFILES = totem.spec

DISTCLEANFILES = intltool-extract intltool-merge intltool-update \
		 mate-doc-utils.make

DIST_SUBDIRS = $(NORMAL_SUBDIRS) browser-plugin
DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc --with-cajadir='$${libdir}/caja/extensions-2.0-distcheck' --enable-vala=yes

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --after="Mon Jul 20 23:47:57 2009" > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

