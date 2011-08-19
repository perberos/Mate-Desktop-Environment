themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = mimetypes

icondir = $(themedir)/$(size)/$(context)

icon_DATA =					\
	application-x-executable.svg \
	audio-x-generic.svg \
	font-x-generic.svg \
	image-x-generic.svg \
	text-html.svg \
	text-x-generic.svg \
	text-x-generic-template.svg \
	text-x-preview.svg \
	text-x-script.svg \
	video-x-generic.svg \
	x-office-address-book.svg \
	x-office-calendar.svg \
	x-office-document.svg \
	x-office-presentation.svg \
	x-office-spreadsheet.svg

placeholder_FILES = 

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
