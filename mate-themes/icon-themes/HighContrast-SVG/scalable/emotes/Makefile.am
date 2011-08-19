themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = emotes

icondir = $(themedir)/$(size)/$(context)

icon_DATA = \
	face-cool.svg \
	face-plain.svg \
	face-sad.svg \
	face-smile.svg \
	face-smile-big.svg \
	face-surprise.svg \
	face-wink.svg

placeholder_FILES = \
	face-angel.svg \
	face-angry.svg \
	face-crying.svg \
	face-devilish.svg \
	face-embarrassed.svg \
	face-kiss.svg \
	face-laugh.svg \
	face-monkey.svg \
	face-raspberry.svg \
	face-sick.svg \
	face-smirk.svg \
	face-tired.svg \
	face-uncertain.svg \
	face-worried.svg

if PLACEHOLDERS
icon_DATA += $(placeholder_FILES)
endif

EXTRA_DIST = \
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
