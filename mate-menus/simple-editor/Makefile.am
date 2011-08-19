SUBDIRS = GMenuSimpleEditor

bin_SCRIPTS = gmenu-simple-editor

gmenu-simple-editor: gmenu-simple-editor.in
	$(AM_V_GEN)sed -e s!\@pyexecdir\@!@pyexecdir@! -e s!\@PYTHON\@!@PYTHON@! < $< > $@ && \
	chmod a+x $@

uidir = $(pkgdatadir)/ui
ui_DATA = gmenu-simple-editor.ui

desktopdir = $(datadir)/applications
desktop_in_files = gmenu-simple-editor.desktop.in 
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST =				\
	gmenu-simple-editor.in		\
	$(desktop_in_files)		\
	$(ui_DATA)

CLEANFILES =				\
	gmenu-simple-editor		\
	$(desktop_DATA)

-include $(top_srcdir)/git.mk
