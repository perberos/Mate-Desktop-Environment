THEME_NAME=LowContrast
THEME_IN_FILES=index.theme.in

@INTLTOOL_THEME_RULE@ 

themedir = $(datadir)/themes/$(THEME_NAME)

theme_DATA = index.theme

DISTCLEANFILES=index.theme
EXTRA_DIST=index.theme.in

if CORE_THEMES_ONLY
install: install-am
	mv $(DESTDIR)$(themedir)/index.theme $(DESTDIR)$(themedir)/index.theme.disabled
uninstall: uninstall-am
	rm -f $(DESTDIR)$(themedir)/index.theme.disabled
endif
