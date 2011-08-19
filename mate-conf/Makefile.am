
SUBDIRS = mateconf backends po doc examples

DIST_SUBDIRS = tests mateconf backends po doc examples defaults gsettings

if ENABLE_DEFAULTS_SERVICE
SUBDIRS += defaults
endif

if ENABLE_GSETTINGS_BACKEND
SUBDIRS += gsettings
endif

EXTRA_DIST =                \
	TODO                \
	gtk-doc.make        \
	intltool-update.in  \
	intltool-extract.in \
	intltool-merge.in

DISTCLEANFILES =          \
	intltool-update   \
	intltool-merge    \
	intltool-extract

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --enable-introspection --enable-defaults-service --enable-gsettings-backend

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
