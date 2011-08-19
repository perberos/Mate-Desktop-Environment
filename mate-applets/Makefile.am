
gtop_applets = multiload
if BUILD_GTOP_APPLETS
gtop_SUBDIRS = $(gtop_applets)
endif

libmateweather_applets = \
	mateweather
if BUILD_LIBMATEWEATHER_APPLETS
libmateweather_SUBDIRS = $(libmateweather_applets)
endif

modemlights_applets = \
	modemlights
if APPLET_MODEMLIGHTS
modemlights_SUBDIRS = $(modemlights_applets)
endif

if BUILD_BATTSTAT_APPLET
battstat_SUBDIR = battstat
endif

if APPLET_ACCESSX
accessx_status_SUBDIR = accessx-status
endif

if BUILD_CPUFREQ_APPLET
cpufreq_SUBDIR = cpufreq
endif

if BUILD_INVEST_APPLET
invest_applet_SUBDIR = invest-applet
endif

if BUILD_MIXER_APPLET
mixer_applet_SUBDIR = mixer
endif

always_built_SUBDIRS =  \
	charpick	\
	drivemount	\
	geyes		\
	mini-commander	\
	gkb-new		\
	stickynotes	\
	trashapplet	\
	null_applet

SUBDIRS = \
	po			\
	$(always_built_SUBDIRS)	\
	$(modemlights_SUBDIRS)	\
	$(gtop_SUBDIRS)		\
	$(libmateweather_SUBDIRS)	\
	$(vfs_SUBDIRS)		\
	$(battstat_SUBDIR)	\
	$(accessx_status_SUBDIR)\
	$(invest_applet_SUBDIR)	\
	$(cpufreq_SUBDIR) \
	$(mixer_applet_SUBDIR)

DIST_SUBDIRS = \
	po		\
	modemlights	\
	drivemount	\
	charpick	\
	geyes		\
	mini-commander	\
	gkb-new		\
	battstat	\
	multiload	\
	mateweather 	\
	man		\
	accessx-status	\
	stickynotes	\
	gswitchit	\
	trashapplet	\
	cpufreq		\
	invest-applet	\
	mixer 		\
	null_applet

EXTRA_DIST = \
	MAINTAINERS			\
	m4/ac-define-dir.m4		\
	COPYING-DOCS			\
	mate-applets.spec.in		\
	mate-applets.spec		\
	intltool-update.in		\
	intltool-extract.in		\
	intltool-merge.in		\
	xmldocs.make			\
	omf.make			\
	mate-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS =	\
	--disable-scrollkeeper \
	--enable-mini-commander \
	--enable-mixer-applet

# We build the ChangeLog from the git log
ChangeLog:
	@if test -f $(top_srcdir)/.git/HEAD; then \
		git log --stat --no-color --since=2009-05-23 > $@; \
	fi

dist: ChangeLog

# Make sure the ChangeLog is updated every time
.PHONY: ChangeLog

# hack from sound-juicer to get around scrollkeeper's leaving a big
# mess behind.
distuninstallcheck_listfiles = 	\
        find -regex '.*/var/scrollkeeper/.*' -prune -or -type f -print

ACLOCAL_AMFLAGS = -I m4

DISTCLEANFILES = intltool-extract	\
		 intltool-merge		\
		 intltool-update	\
		 mate-doc-utils.make

-include $(top_srcdir)/git.mk
