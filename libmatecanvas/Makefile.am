AUTOMAKE_OPTIONS = 1.7

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

SUBDIRS = libmatecanvas glade demos po docs

if USE_GLADE
SUBDIRS += tests
endif

EXTRA_DIST = \
	COPYING.LIB		\
	NEWS			\
	intltool-extract.in	\
	intltool-merge.in	\
	intltool-update.in	\
	libmatecanvas-zip.in

DISTCLEANFILES = \
	intltool-extract \
	intltool-merge \
	intltool-update
