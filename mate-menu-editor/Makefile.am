## Process this file with automake to produce Makefile.in

SUBDIRS = data po Alacarte

CLEANFILES=

bin_SCRIPTS = alacarte
CLEANFILES += alacarte

DISTCLEANFILES = ChangeLog

alacarte: alacarte.in
	sed -e s!\@PYTHON\@!@PYTHON@!		\
	    -e s!\@PYOPTIONS\@!-OOt!		\
	    < $< > $@
alacarte: Makefile

EXTRA_DIST = \
	alacarte.in \
	MAINTAINERS \
	ChangeLog.pre-git

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run git log git-migration.. --stat -M -C --name-status  --date=short --no-color) | fmt --split-only > $@.tmp \
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
	
