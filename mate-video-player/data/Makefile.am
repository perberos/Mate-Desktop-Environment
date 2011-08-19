SUBDIRS = icons

bugreportdir = $(libexecdir)/totem
bugreport_SCRIPTS = totem-bugreport.py

man_MANS = totem.1 totem-video-thumbnailer.1

stuffdir = $(pkgdatadir)
stuff_DATA =				\
	filmholes.png			\
	filmholes-big-left.png		\
	filmholes-big-right.png		\
	totem.ui			\
	fullscreen.ui			\
	playlist.ui			\
	plugins.ui			\
	mozilla-viewer.ui		\
	properties.ui			\
	uri.ui				\
	video-list.ui

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

# totem.desktop.in.in.in = original
# totem.desktop.in.in = by configure, includes the X-Mate-Bugzilla
# totem.desktop.in = includes the mime-types
# totem.desktop = intltool-ised

desktop_in_files = totem.desktop.in
desktopdir = $(datadir)/applications
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

totem.desktop.in: totem.desktop.in.in mime-type-list.txt desktop.sh
	$(AM_V_GEN) cat totem.desktop.in.in | sed 's,@FULL_LIBEXECDIR@,$(FULL_LIBEXECDIR),' > $@ &&\
	$(SHELL) $(srcdir)/desktop.sh $(srcdir)/mime-type-list.txt	\
	>> $@

nodist_noinst_HEADERS = totem-mime-types.h caja-audio-mime-types.h
totem-mime-types.h: mime-type-include.sh mime-type-list.txt mime-functions.sh
	$(AM_V_GEN) $(srcdir)/mime-type-include.sh $(srcdir)/mime-type-list.txt	\
	> $@
caja-audio-mime-types.h: mime-type-include.sh mime-type-list.txt mime-functions.sh
	$(AM_V_GEN) $(srcdir)/mime-type-include.sh --caja $(srcdir)/mime-type-list.txt	\
	> $@

update-mateconf.sh: totem-video-thumbnail.schemas.in

totem-video-thumbnail.schemas.in: schemas.sh mime-type-list.txt mime-functions.sh
	$(AM_V_GEN) $(srcdir)/schemas.sh $(srcdir)/mime-type-list.txt $(bindir)	\
	> $@								\
	3> update-mateconf.sh

totem-handlers.schemas.in: handlers.sh
	$(AM_V_GEN) $(srcdir)/handlers.sh > $@

generated_schema_in_files = totem-video-thumbnail.schemas.in totem-handlers.schemas.in
schema_in_files = totem.schemas.in $(generated_schema_in_files)
schemadir = $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

totem-video-thumbnailer.1: totem-video-thumbnailer.pod
	$(AM_V_GEN) pod2man -c "" -s 1 -q none -n totem-video-thumbnailer -r MATE $< > $@

install-data-local: $(schema_DATA)
if MATECONF_SCHEMAS_INSTALL
	if test -z "$(DESTDIR)" ; then \
	for p in $^ ; do \
		MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p 2>&1 > /dev/null; \
	done \
	fi
endif

EXTRA_DIST = \
	$(man_MANS)			\
	$(stuff_DATA)			\
	$(bugreport_SCRIPTS)		\
	desktop.sh			\
	handlers.sh			\
	mime-type-include.sh		\
	mime-type-list.txt		\
	schemas.sh			\
	mime-functions.sh		\
	totem.schemas.in		\
	totem-video-thumbnailer.1	\
	totem-video-thumbnailer.pod

CLEANFILES = \
	*.bak *.uip core* *.orig *~ 	\
	$(desktop_in_files)		\
	$(desktop_DATA)			\
	$(generated_schema_in_files)	\
	$(schema_DATA)			\
	totem-handlers.schemas.in 	\
	totem-mime-types.h		\
	totem-video-thumbnail.schemas.in\
	caja-audio-mime-types.h	\
	update-mateconf.sh

DISTCLEANFILES =
