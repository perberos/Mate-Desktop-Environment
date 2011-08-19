themedir = $(datadir)/icons/Mist
size = 32x32
context = status

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA =			\
	folder-visiting.png	\
	folder-drag-accept.png	\
	folder-open.png

EXTRA_DIST =		\
	$(icons_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconsDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
