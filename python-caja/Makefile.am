SUBDIRS = src examples

if ENABLE_GTK_DOC
SUBDIRS += docs
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = caja-python.pc

EXTRA_DIST = caja-python.pc.in

DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc
