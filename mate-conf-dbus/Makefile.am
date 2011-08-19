
SUBDIRS = mateconf backends po doc examples dbus-tests
DIST_SUBDIRS=tests $(SUBDIRS)

EXTRA_DIST =                \
	TODO                \
	gtk-doc.make        \
	intltool-update.in  \
	intltool-extract.in \
	intltool-merge.in   \
	autogen.sh

DISTCLEANFILES =          \
	intltool-update   \
	intltool-merge    \
	intltool-extract

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

install-data-local:
	-mkdir -p $(DESTDIR)$(sysmateconfdir)/mateconf.xml.defaults
	-chmod 755 $(DESTDIR)$(sysmateconfdir)/mateconf.xml.defaults
	-mkdir -p $(DESTDIR)$(sysmateconfdir)/mateconf.xml.mandatory
	-chmod 755 $(DESTDIR)$(sysmateconfdir)/mateconf.xml.mandatory

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mateconf-2.0.pc

aclocaldir = $(datadir)/aclocal
aclocal_DATA = mateconf-2.m4

install-schemas:
	(cd standard-schemas && $(MAKE) $(AM_MAKEFLAGS) install-schemas)
