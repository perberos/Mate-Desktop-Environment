SUBDIRS = plugins

musicappletdir = $(pythondir)/musicapplet
musicapplet_PYTHON = 	\
	__init__.py	\
	applet.py	\
	conf.py		\
	manager.py	\
	player.py	\
	util.py
nodist_musicapplet_PYTHON = \
	defs.py

musicappletexecdir = $(pyexecdir)/musicapplet
musicappletexec_LTLIBRARIES = widgets.la
widgets_la_LDFLAGS = -module -avoid-version -export-symbols-regex initwidgets
widgets_la_SOURCES = 		\
	ma-constrain.c		\
	ma-deserialize.c	\
	ma-fancy-button.c	\
	ma-fancy-tooltips.c	\
	ma-scroller.c		\
	widgetsmodule.c
nodist_widgets_la_SOURCES =	\
	widgets.c

noinst_HEADERS = 		\
	ma-constrain.h		\
	ma-deserialize.h	\
	ma-fancy-button.h	\
	ma-fancy-tooltips.h	\
	ma-scroller.h

widgets.defs: ma-constrain.h ma-deserialize.h ma-fancy-button.h ma-fancy-tooltips.h ma-scroller.h
	$(PYTHON) $(PYGTK_H2DEF) $^ >$@

widgets.c: widgets.defs widgets.override
	$(PYGTK_CODEGEN) --prefix widgets				\
			 --register $(PYGTK_DEFSDIR)/gdk-types.defs	\
			 --register $(PYGTK_DEFSDIR)/gtk-types.defs	\
			 --override $(srcdir)/widgets.override			\
			 widgets.defs >$@

defs.py: defs.py.in
	sed -e "s|@xDATADIRx@|$(datadir)|" 		\
	    -e "s|@xLIBDIRx@|$(libdir)|"		\
	    -e "s|@xPYTHONDIRx@|$(pythondir)|"		\
	    -e "s|@xPKGDATADIRx@|$(pkgdatadir)|" 	\
	    $< > $@

BUILT_SOURCES = 		\
	defs.py

EXTRA_DIST = 			\
	defs.py.in.in		\
	widgets.override

CLEANFILES = 			\
	$(BUILT_SOURCES)	\
	widgets.c		\
	widgets.defs
