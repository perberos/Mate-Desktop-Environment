accessx_status_iconsdir = $(iconsdir)/hicolor/48x48/apps
accessx_status_icons_DATA = ax-applet.png

accessx_status_pixmapsdir  = $(pixmapsdir)/accessx-status-applet
accessx_status_pixmaps_DATA =	\
	ax-applet.png			\
	ax-key-base.png			\
	ax-key-inverse.png		\
	ax-key-none.png			\
	ax-key-yes.png			\
	ax-key-no.png			\
	mousekeys-base.png		\
	mousekeys-pressed-left.png	\
	mousekeys-pressed-middle.png	\
	mousekeys-pressed-right.png	\
	mousekeys-default-left.png	\
	mousekeys-default-middle.png	\
	mousekeys-default-right.png	\
	sticky-shift-none.png		\
	sticky-shift-latched.png	\
	sticky-shift-locked.png		\
	sticky-ctrl-none.png		\
	sticky-ctrl-latched.png		\
	sticky-ctrl-locked.png		\
	sticky-alt-none.png		\
	sticky-alt-latched.png		\
	sticky-alt-locked.png		\
	sticky-meta-latched.png		\
	sticky-meta-locked.png		\
	sticky-meta-none.png		\
	sticky-hyper-latched.png		\
	sticky-hyper-locked.png		\
	sticky-hyper-none.png		\
	sticky-super-latched.png		\
	sticky-super-locked.png		\
	sticky-super-none.png		\
	ax-slowkeys.png			\
	ax-slowkeys-pending.png		\
	ax-slowkeys-yes.png		\
	ax-slowkeys-no.png		\
	ax-bouncekeys.png	

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = \
	$(accessx_status_pixmaps_DATA)

-include $(top_srcdir)/git.mk
