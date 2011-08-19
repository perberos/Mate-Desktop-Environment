themedir =  $(datadir)/icons/HighContrast-SVG
size = scalable
context = actions

icondir = $(themedir)/$(size)/$(context)

icon_DATA = \
	application-exit.svg \
	contact-new.svg \
	dialog-cancel.svg \
	dialog-close.svg \
	dialog-ok.svg \
	document-new.svg \
	document-open-recent.svg \
	document-open.svg \
	document-print-preview.svg \
	document-print.svg \
	document-properties.svg \
	document-revert.svg \
	document-save-as.svg \
	document-save.svg \
	edit-clear.svg \
	edit-copy.svg \
	edit-cut.svg \
	edit-delete.svg \
	edit-find-replace.svg \
	edit-find.svg \
	edit-paste.svg \
	edit-redo.svg \
	edit-undo.svg \
	folder-new.svg \
	format-indent-less.svg \
	format-indent-more.svg \
	format-justify-center.svg \
	format-justify-fill.svg \
	format-justify-left.svg \
	format-justify-right.svg \
	format-text-bold.svg \
	format-text-italic.svg \
	format-text-strikethrough.svg \
	format-text-underline.svg \
	go-bottom.svg \
	go-down.svg \
	go-first.svg \
	go-home.svg \
	go-jump.svg \
	go-last.svg \
	go-next.svg \
	go-previous.svg \
	go-top.svg \
	go-up.svg \
	help-about.svg \
	help-contents.svg \
	help-faq.svg \
	insert-image.svg \
	insert-object.svg \
	insert-text.svg \
	list-add.svg \
	list-remove.svg \
	mail-forward.svg \
	mail-mark-important.svg \
	mail-mark-read.svg \
	mail-mark-unread.svg \
	mail-message-new.svg \
	mail-reply-all.svg \
	mail-reply-sender.svg \
	mail-send-receive.svg \
	media-eject.svg \
	media-playback-pause.svg \
	media-playback-start.svg \
	media-playback-stop.svg \
	media-record.svg \
	media-seek-backward.svg \
	media-seek-forward.svg \
	media-skip-backward.svg \
	media-skip-forward.svg \
	object-flip-horizontal.svg \
	object-flip-vertical.svg \
	object-rotate-left.svg \
	object-rotate-right.svg \
	process-stop.svg \
	system-lock-screen.svg \
	system-log-out.svg \
	system-run.svg \
	system-search.svg \
	system-shutdown.svg \
	tool-check-spelling.svg \
	tools-check-spelling.svg \
	view-fullscreen.svg \
	view-refresh.svg \
	view-restore.svg \
	view-sort-ascending.svg \
	view-sort-descending.svg \
	window-close.svg \
	window-new.svg \
	zoom-fit-best.svg \
	zoom-in.svg \
	zoom-original.svg \
	zoom-out.svg

placeholder_FILES = \
	address-book-new.svg \
	appointment-new.svg \
	call-start.svg \
	call-stop.svg \
	document-page-setup.svg \
	document-send.svg \
	edit-select-all.svg \
	format-text-direction-ltr.svg \
	format-text-direction-rtl.svg \
	insert-link.svg \
	mail-mark-junk.svg \
	mail-mark-notjunk.svg \
	mail-send.svg \
	system-reboot.svg

if PLACEHOLDERS
icon_DATA += $(placeholder_FILES)
endif


EXTRA_DIST =				\
	$(icon_DATA)	\
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
