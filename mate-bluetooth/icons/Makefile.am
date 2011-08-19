
icons = hicolor_apps_16x16_bluetooth.png \
	hicolor_apps_22x22_bluetooth.png \
	hicolor_apps_24x24_bluetooth.png \
	hicolor_apps_32x32_bluetooth.png \
	hicolor_apps_48x48_bluetooth.png \
	hicolor_apps_scalable_bluetooth.svg \
	hicolor_status_16x16_bluetooth-active.png \
	hicolor_status_22x22_bluetooth-active.png \
	hicolor_status_24x24_bluetooth-active.png \
	hicolor_status_32x32_bluetooth-active.png \
	hicolor_status_48x48_bluetooth-active.png \
	hicolor_status_16x16_bluetooth-disabled.png \
	hicolor_status_22x22_bluetooth-disabled.png \
	hicolor_status_24x24_bluetooth-disabled.png \
	hicolor_status_32x32_bluetooth-disabled.png \
	hicolor_status_48x48_bluetooth-disabled.png \
	hicolor_status_16x16_bluetooth-paired.png \
	hicolor_status_22x22_bluetooth-paired.png \
	hicolor_status_24x24_bluetooth-paired.png \
	hicolor_status_32x32_bluetooth-paired.png \
	hicolor_status_scalable_bluetooth-paired.svg


noinst_DATA = hicolor_apps_16x16_bluetooth.svg \
		hicolor_apps_22x22_bluetooth.svg \
		hicolor_apps_32x32_bluetooth.svg \
		hicolor_status_16x16_bluetooth-active.svg \
		hicolor_status_22x22_bluetooth-active.svg \
		hicolor_status_32x32_bluetooth-active.svg \
		hicolor_status_48x48_bluetooth-active.svg \
		hicolor_status_16x16_bluetooth-disabled.svg \
		hicolor_status_22x22_bluetooth-disabled.svg \
		hicolor_status_32x32_bluetooth-disabled.svg \
		hicolor_status_48x48_bluetooth-disabled.svg

EXTRA_DIST = $(icons) $(noinst_DATA)

MAINTAINERCLEANFILES = Makefile.in

install-data-local:
	for i in $(icons); do \
		THEME=`echo $$i | cut -d_ -f1`; \
		CONTEXT=`echo $$i | cut -d_ -f2`; \
		SIZE=`echo $$i | cut -d_ -f3`; \
		FILE=`echo $$i | cut -d_ -f4`; \
		mkdir -p $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT; \
		$(INSTALL_DATA) $(srcdir)/$$i $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$FILE; \
	done

uninstall-local:
	for i in $(icons); do \
		THEME=`echo $$i | cut -d_ -f1`; \
		CONTEXT=`echo $$i | cut -d_ -f2`; \
		SIZE=`echo $$i | cut -d_ -f3`; \
		FILE=`echo $$i | cut -d_ -f4`; \
		rm -f $(DESTDIR)$(datadir)/icons/$$THEME/$$SIZE/$$CONTEXT/$$FILE; \
	done

install-data-hook:
if ICON_UPDATE
	$(UPDATE_ICON_CACHE) -f -t $(DESTDIR)$(datadir)/icons/hicolor
endif

uninstall-hook:
if ICON_UPDATE
	$(UPDATE_ICON_CACHE) $(DESTDIR)$(datadir)/icons/hicolor
endif

