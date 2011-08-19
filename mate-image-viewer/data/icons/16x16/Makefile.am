size = 16x16
actioniconsdir = $(pkgdatadir)/icons/hicolor/$(size)/actions
appiconsdir = $(datadir)/icons/hicolor/$(size)/apps

dist_appicons_DATA = \
	apps/eog.png
dist_actionicons_DATA = \
	actions/eog-plugin.png	  	\
	actions/eog-image-collection.png  \
	actions/slideshow-play.png

noinst_DATA = \
	apps/eog.svg		\
	actions/eog-plugin.svg  \
	actions/slideshow-play.svg

EXTRA_DIST = $(noinst_DATA)

-include $(top_srcdir)/git.mk
