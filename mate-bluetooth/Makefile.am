DEF_SUBDIRS = icons lib applet properties wizard sendto docs po help

if WITH_MOBLIN
MOBLIN_DIR = moblin
endif

SUBDIRS = icons lib applet properties wizard sendto $(MOBLIN_DIR) docs po help

DIST_SUBDIRS = $(DEF_SUBDIRS) moblin
EXTRA_DIST = intltool-extract.in intltool-update.in intltool-merge.in ChangeLog.pre-2.27 gtk-doc.make mate-doc-utils.make

# FIXME https://bugzilla.mate.org/show_bug.cgi?id=595675
DISTCHECK_CONFIGURE_FLAGS = --disable-schemas-install \
	--disable-desktop-update --disable-mime-update --disable-icon-update \
	--enable-gtk-doc --disable-scrollkeeper --enable-introspection

DISTCLEANFILES = intltool-extract intltool-update intltool-merge mate-doc-utils.make

MAINTAINERCLEANFILES = Makefile.in \
	aclocal.m4 configure config.h.in config.sub config.guess \
	ltmain.sh depcomp missing install-sh mkinstalldirs \
	intltool-extract.in intltool-update.in intltool-merge.in \
	mate-bluetooth-1.0.pc

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-bluetooth-1.0.pc

install-data-hook:
if DESKTOP_UPDATE
	$(UPDATE_DESKTOP_DATABASE) $(DESTDIR)$(datadir)/applications
endif

uninstall-hook:
if DESKTOP_UPDATE
	$(UPDATE_DESKTOP_DATABASE) $(DESTDIR)$(datadir)/applications
endif

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --after="Fri Feb 20 00:08:02 2009" > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

