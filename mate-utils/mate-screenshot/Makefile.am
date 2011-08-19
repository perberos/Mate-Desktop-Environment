NULL =

INCLUDES =							\
	-I.							\
	-I$(srcdir)						\
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DUIDIR=\""$(uidir)"\"					\
	$(MATE_UTILS_CFLAGS)					\
	$(LIBCANBERRA_GTK_CFLAGS)				\
	$(GTHREAD_CFLAGS)					\
	$(NULL)

bin_PROGRAMS = \
	mate-screenshot		\
	$(NULL)

mate_screenshot_SOURCES = \
	mate-screenshot.c		\
	screenshot-dialog.c		\
	screenshot-dialog.h		\
	screenshot-shadow.c		\
	screenshot-shadow.h		\
	screenshot-utils.c		\
	screenshot-utils.h		\
	screenshot-save.c		\
	screenshot-save.h		\
	screenshot-xfer.c		\
	screenshot-xfer.h		\
	$(NULL)

mate_screenshot_LDFLAGS = -export-dynamic

mate_screenshot_LDADD =		\
	$(XSHAPE_LIBS)			\
	$(MATE_UTILS_LIBS)		\
	$(LIBCANBERRA_GTK_LIBS)		\
	$(GTHREAD_LIBS)			\
	-lm				\
	$(NULL)

mate_screenshotdir  = $(datadir)/applications
mate_screenshot_in_files = mate-screenshot.desktop.in
mate_screenshot_DATA = $(mate_screenshot_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

man_MANS = mate-screenshot.1

uidir = $(datadir)/mate-screenshot
ui_DATA =					\
	mate-screenshot.ui			\
	$(NULL)

EXTRA_DIST =					\
	$(schemas_in_files)			\
	$(mate_screenshot_in_files)		\
	$(man_MANS)				\
	$(ui_DATA)				\
	$(NULL)

schemasdir       = $(MATECONF_SCHEMA_FILE_DIR)
schemas_in_files =				\
	mate-screenshot.schemas.in		\
	$(NULL)
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

@INTLTOOL_SCHEMAS_RULE@

if MATECONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/mate-screenshot/$$p ; \
		done ; \
	fi
else
install-data-local:
endif

CLEANFILES = \
	$(BUILT_SOURCES)			 \
	$(schemas_in_files:.schemas.in=.schemas) \
	$(mate_screenshot_DATA)		 \
	$(sys_DATA)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)

install-exec-local:
	rm -f $(DESTDIR)$(bindir)/mate-panel-screenshot
	ln -s mate-screenshot $(DESTDIR)$(bindir)/mate-panel-screenshot
