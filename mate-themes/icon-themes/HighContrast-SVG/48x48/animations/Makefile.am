themedir =  $(datadir)/icons/HighContrast-SVG
size = 48x48
context = animations

icondir = $(themedir)/$(size)/$(context)

icon_DATA =			\
	process-idle.png	\
	process-working.png

EXTRA_DIST =		\
	$(icon_DATA)

MAINTAINERCLEANFILES =	\
	Makefile.in

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif

uninstall-symlinks:
	list=`ls $(DESTDIR)$(themedir)/$(size)/$(context)`; \
	for p in $$list; do \
		if test -h $(DESTDIR)$(themedir)/$(size)/$(context)/$$p; then \
			rm -f $(DESTDIR)$(themedir)/$(size)/$(context)/$$p; \
		fi; \
	done

uninstall-am: uninstall-iconDATA uninstall-symlinks
