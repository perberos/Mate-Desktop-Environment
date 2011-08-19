themedir = $(THEME_DIR)
theme_DATA = bluetooth-panel.css

%.css: %.css.in $(top_builddir)/config.log
	$(QUIET_GEN)sed -e "s|\@pkgdatadir\@|$(pkgdatadir)|" $< > $@

EXTRA_DIST = bluetooth-panel.css.in
DISTCLEANFILES = bluetooth-panel.css
