SUBDIRS = src docs po

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --with-cajadir='$${libdir}/caja/extensions-2.0-distcheck'

man_MANS = caja-sendto.1

EXTRA_DIST =                    \
        AUTHORS                 \
	NEWS                    \
	README                  \
	ChangeLog		\
	ChangeLog.pre-1.1.4.1	\
	intltool-merge.in       \
	intltool-update.in      \
	intltool-extract.in	\
	$(man_MANS)             \
	$(schema_in_files)

CLEANFILES =			\
	$(schema_DATA)

DISTCLEANFILES =                        \
	po/.intltool-merge-cache        \
	intltool-extract                \
	intltool-merge                  \
	intltool-update

# Build ChangeLog from GIT history
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --after="Fri Apr 17 16:36:52 2009" > $@; \
	fi

