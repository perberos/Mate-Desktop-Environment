themedir = $(datadir)/icons/Mist
size = 16x16
context = places

iconsdir = $(themedir)/$(size)/$(context)

icons_DATA =			\
	folder.png		\
	folder-new.png		\
	folder-remote.png	\
	folder-saved-search.png	\
	user-home.png		\
	folder-documents.png	\
	folder-download.png	\
	folder-music.png	\
	folder-pictures.png	\
	folder-publicshare.png	\
	folder-templates.png	\
	folder-videos.png	\
	user-desktop.png

EXTRA_DIST =		\
	$(icons_DATA)

if LEGACY_ICON_MAPPING
install-data-local: install-iconsDATA
	(cd $(DESTDIR)$(themedir)/$(size) && $(ICONMAP) -c $(context))
endif
