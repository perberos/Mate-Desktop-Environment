SUBDIRS = libmate monikers schemas mate-data po doc

EXTRA_DIST = \
	ChangeLog.pre-git	\
	COPYING.LIB		\
	NEWS			\
	MAINTAINERS		\
	libmate-zip.in		\
	gtk-doc.make

DISTCLEANFILES =

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog
