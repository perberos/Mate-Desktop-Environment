#
# FIXME:
#  currently scrollkeeper-update does not seem to
#  remove its database when there is no useful
#  information contained there. Therefore, with
#  "make uninstall" you're left with loads of
#  stuff in /var/scrollkeeper and distcheck
#  fails.
#
#  http://mail.mate.org/archives/mate-doc-list/2003-May/msg00008.html
#
#  This overrides the uninstall check so distcheck
#  passes
#
# distuninstallcheck_listfiles = find . -type f -print | grep -v 'scrollkeeper' | grep -v 'omf' | grep -v 'figures'
distuninstallcheck:
	@:

distcleancheck_listfiles = find . -type f -print | grep -v 'omf\.out' | grep -v 'legal.xml'

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = \
	m4			\
	po			\
	libeggsmclient		\
	baobab			\
	mate-dictionary	\
	mate-screenshot	\
	gsearchtool		\
	logview			\
	$(NULL)

EXTRA_DIST = \
	omf.make \
	xmldocs.make \
	mate-doc-utils.make \
	COPYING.libs \
	COPYING.docs

DISTCLEANFILES =                \
	gtk-doc.make		\
	mate-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-gtk-doc

dist-hook:
	@if test -d "$(srcdir)/.git"; \
	then \
		from="git-import"; \
		echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && \
		  echo '# Generate by configure. Do not edit.'; echo; \
		  $(top_srcdir)/missing --run git log --stat $$from.. ) > ChangeLog.tmp \
		&& mv -f ChangeLog.tmp $(top_distdir)/ChangeLog \
		|| ( rm -f ChangeLog.tmp ; \
		     echo Failed to generate ChangeLog >&2 ); \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi
