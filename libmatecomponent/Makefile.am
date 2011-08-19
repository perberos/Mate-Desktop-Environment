SUBDIRS = \
	idl matecomponent-activation matecomponent \
	activation-server monikers \
	utils samples tests doc po

libincludedir = $(includedir)/libmatecomponent-2.0
libinclude_HEADERS = \
	libmatecomponent.h

EXTRA_DIST = \
	MAINTAINERS \
	matecomponent-activation-config.xml \
	libmatecomponent-zip.in     \
	intltool-extract.in  \
	intltool-merge.in    \
	intltool-update.in   \
	changes.txt          \
	gtk-doc.make

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

install-data-local:
	-mkdir -p $(DESTDIR)$(sysconfdir)/matecomponent-activation
	if test -f $(DESTDIR)$(sysconfdir)/matecomponent-activation/matecomponent-activation-config.xml; then	\
		echo "matecomponent-activation-config.xml configuration file not changed";		\
	else											\
		$(INSTALL_DATA) $(top_srcdir)/matecomponent-activation-config.xml			\
			$(DESTDIR)$(sysconfdir)/matecomponent-activation;				\
	fi

uninstall-local:
	if cmp $(DESTDIR)$(sysconfdir)/matecomponent-activation/matecomponent-activation-config.xml $(top_srcdir)/matecomponent-activation-config.xml > /dev/null ; then \
		rm -f $(DESTDIR)$(sysconfdir)/matecomponent-activation/matecomponent-activation-config.xml; \
	fi

DISTCLEANFILES=intltool-extract intltool-merge intltool-update
