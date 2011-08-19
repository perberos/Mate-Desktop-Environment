themedir = $(datadir)/icons/Crux
size = 16x16
context = actions

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA =			\
	document-open.png	\
	go-bottom.png		\
	go-first.png		\
	go-last.png		\
	go-previous.png		\
	go-up.png		\
	go-down.png		\
	go-home.png		\
	go-next.png		\
	go-top.png		\
	view-refresh.png


EXTRA_DIST =		\
	$(icons_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconsDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
