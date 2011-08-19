themedir = $(datadir)/icons/Crux
size = 24x24
context = actions

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA =			\
	document-open.png	\
	go-bottom.png		\
	go-down.png		\
	go-first.png		\
	go-home.png		\
	go-last.png		\
	go-next.png		\
	go-previous.png		\
	go-top.png		\
	go-up.png		\
	view-refresh.png


EXTRA_DIST =		\
	$(icons_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconsDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
