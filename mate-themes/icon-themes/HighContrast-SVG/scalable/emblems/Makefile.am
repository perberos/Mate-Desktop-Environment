themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = emblems

icondir = $(themedir)/$(size)/$(context)

icon_DATA =				\
	emblem-default.svg \
	emblem-documents.svg \
	emblem-downloads.svg \
	emblem-favorite.svg \
	emblem-important.svg \
	emblem-mail.svg \
	emblem-photos.svg \
	emblem-readonly.svg \
	emblem-symbolic-link.svg \
	emblem-system.svg \
	emblem-unreadable.svg

placeholder_FILES = \
	emblem-shared.svg \
	emblem-synchronized.svg

if PLACEHOLDERS
icon_DATA += $(placeholder_FILES)
endif

EXTRA_DIST =				\
	$(icon_DATA) \
	$(placeholder_FILES)

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
