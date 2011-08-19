themedir = $(datadir)/icons/Crux
size = scalable
context = actions

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA =			\
	document-open.svg	\
	go-bottom.svg		\
	go-down.svg		\
	go-first.svg		\
	go-home.svg		\
	go-last.svg		\
	go-next.svg		\
	go-previous.svg		\
	go-top.svg		\
	go-up.svg		\
	view-refresh.svg


EXTRA_DIST =		\
	$(icons_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconsDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
