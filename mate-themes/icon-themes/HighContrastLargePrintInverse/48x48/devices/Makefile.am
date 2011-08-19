NULL=

themedir = $(datadir)/icons/HighContrastLargePrintInverse
size = 48x48
context = devices

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		computer.png \
		drive-harddisk.png \
		drive-optical.png \
		drive-removable-media.png \
		mate-dev-printer-network.png \
		mate-dev-printer-new.png \
		input-keyboard.png \
		input-mouse.png \
		media-floppy.png \
		media-optical.png \
		network-wireless.png \
		pda.png \
		printer.png \
		$(NULL)

EXTRA_DIST=	$(icon_DATA) \
		symlinks.dat

if LEGACY_ICON_MAPPING
install-data-local: install-iconDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif

install: install-am
	$(SHELL) $(top_srcdir)/common/mkiconlinks.sh $(srcdir)/symlinks.dat $(DESTDIR)$(icondir)

uninstall-symlinks:
	list=`ls $(DESTDIR)$(icondir)`; for p in $$list; do \
	if test -h $(DESTDIR)$(icondir)/$$p; then \
		rm -f $(DESTDIR)$(icondir)/$$p; \
	fi; \
	done

uninstall-am: uninstall-iconDATA uninstall-symlinks
