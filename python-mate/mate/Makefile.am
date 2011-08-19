INCLUDES = $(PYTHON_INCLUDES) $(PYGTK_CFLAGS)

defsdir = $(datadir)/pygtk/2.0/defs
defs_DATA = \
  mate.defs \
  mate-types.defs \
  ui.defs

EXTRA_DIST = $(defs_DATA)

pkgpythondir = $(pyexecdir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

pymatedir = $(pkgpythondir)/mate
pymate_PYTHON = __init__.py canvas.py vfs.py

pymateexecdir = $(pkgpyexecdir)/mate
pymateexec_LTLIBRARIES =

# libmate binding

pymateexec_LTLIBRARIES += _mate.la
_mate_la_CFLAGS = $(MATE_CFLAGS)
_mate_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_mate
_mate_la_LIBADD = $(MATE_LIBS) $(PYTHON_LIBS)
_mate_la_SOURCES = matemodule.c
nodist__mate_la_SOURCES = mate.c
CLEANFILES = mate.c
EXTRA_DIST += mate.override  
mate.c: mate.defs mate.override

# libmateui binding

if BUILD_MATEUI
pymateexec_LTLIBRARIES += ui.la
endif
ui_la_CFLAGS = $(MATEUI_CFLAGS) -I$(top_srcdir)/matevfs
ui_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initui
ui_la_LIBADD = $(MATEUI_LIBS) $(PYTHON_LIBS)
ui_la_SOURCES = uimodule.c
nodist_ui_la_SOURCES = ui.c
CLEANFILES += ui.c
EXTRA_DIST += ui.override  
ui.c: ui.defs ui.override


.defs.c:
	(cd $(srcdir) \
	 && $(PYMATE_CODEGEN) \
	    --py_ssize_t-clean \
	    --load-types ../matecomponent/matecomponent-arg-types.py \
	    --register $(PYGTK_DEFSDIR)/pango-types.defs \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	    --register ../matecomponent/matecomponent-types.defs \
	    --register ../matecomponent/matecomponentui-types.defs \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c


install-exec-hook: # Remove old deprecated modules
	-$(LIBTOOL) --mode=uninstall rm -f $(pymateexecdir)/canvas.la
	-$(LIBTOOL) --mode=uninstall rm -f $(pymateexecdir)/vfs.la

EXTRA_DIST += wscript

