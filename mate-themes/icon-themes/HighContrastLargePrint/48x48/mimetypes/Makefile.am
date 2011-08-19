NULL=

themedir = $(datadir)/icons/HighContrastLargePrint
size = 48x48
context = mimetypes

icondir = $(themedir)/$(size)/$(context)

icon_DATA=	\
		application-x-executable.png \
		audio-x-generic.png \
		font-x-generic.png \
		mate-mime-application-x-core-file.png \
		image-x-generic.png \
		package-x-generic.png \
		text-x-generic.png \
		text-x-preview.png \
		video-x-generic.png \
		x-office-address-book.png \
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
