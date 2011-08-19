SUBDIRS = data src po docs

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh \
	ChangeLog.pre-git \
	MAINTAINERS \
	mate-doc-utils.make

# ignore scrollkeeper files, which will legitimately exist after uninstall.
distuninstallcheck_listfiles = find . -type f -print | grep -v scrollkeeper

DISTCLEANFILES = \
  mate-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

distclean-local:
	if test "$(srcdir)" = "."; then :; else \
		rm -f ChangeLog; \
	fi

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run git log MATECONF_EDITOR_2_26_0.. --stat -M -C --name-status  --date=short --no-color) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	   echo A git checkout and git-log is required to generate this file >> $@); \
	fi

.PHONY: ChangeLog
