NULL =

schema_in_files = \
	desktop_mate_accessibility_keyboard.schemas.in \
	desktop_mate_accessibility_startup.schemas.in \
	desktop_mate_applications_at_visual.schemas.in \
	desktop_mate_applications_at_mobility.schemas.in \
	desktop_mate_applications_browser.schemas.in \
	desktop_mate_applications_terminal.schemas.in \
	desktop_mate_applications_window_manager.schemas.in \
	desktop_mate_applications_office.schemas.in \
	desktop_mate_file_views.schemas.in \
	desktop_mate_lockdown.schemas.in \
	desktop_mate_peripherals_mouse.schemas.in \
	desktop_mate_peripherals_keyboard.schemas.in \
	desktop_mate_sound.schemas.in \
	desktop_mate_typing_break.schemas.in \
	desktop_mate_thumbnail_cache.schemas.in \
	desktop_mate_thumbnailers.schemas.in \
	$(NULL)

desktop_mate_background.schemas.in: desktop_mate_background.schemas.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@datadir\@^$(datadir)^g' < $(srcdir)/desktop_mate_background.schemas.in.in > desktop_mate_background.schemas.in.tmp \
	&& mv desktop_mate_background.schemas.in.tmp desktop_mate_background.schemas.in

schemadir= $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = \
	$(schema_in_files:.schemas.in=.schemas) \
	desktop_mate_background.schemas \
	desktop_mate_interface.schemas \
	$(NULL)

@INTLTOOL_SCHEMAS_RULE@

if MATECONF_SCHEMAS_INSTALL
# don't do this if we are building in eg. rpm
if OS_WIN32
# Work around MSYS feature. If we pass MATECONF_CONFIG_SOURCE as an
# environment variable, MSYS is "helpful" and thinks the colons in the
# variable means that is is a search path, and changes the colons into
# semicolons and slashes into backslashes. Usually this feature is
# useful, but not this time.
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			(echo set MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE); \
			 echo $(MATECONFTOOL) --makefile-install-rule $$p) >_temp.bat; \
			cmd /c _temp.bat; \
		done \
	fi
else
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schema_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $$p; \
		done \
	fi
endif
else
install-data-local:
endif

EXTRA_DIST = \
	$(schema_in_files) \
	desktop_mate_background.schemas.in.in \
	$(NULL)

CLEANFILES = \
	$(schema_DATA) \
	desktop_mate_background.schemas.in \
	$(NULL)
