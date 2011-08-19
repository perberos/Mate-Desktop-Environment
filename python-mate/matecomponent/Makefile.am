
INCLUDES = $(PYTHON_INCLUDES) $(PYGTK_CFLAGS) $(PYMATECORBA_CFLAGS) \
  $(MATECOMPONENT_CFLAGS) $(MATECOMPONENTUI_CFLAGS)

defsdir = $(datadir)/pygtk/2.0/defs
defs_DATA = \
  matecomponent.defs \
  matecomponent-types.defs \
  matecomponentui.defs \
  matecomponentui-types.defs

EXTRA_DIST = $(defs_DATA)

pkgpythondir = $(pyexecdir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0

pymatecomponentdir = $(pkgpythondir)/matecomponent
pymatecomponent_PYTHON = __init__.py

pymatecomponentexecdir = $(pkgpyexecdir)/matecomponent

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

CLEANFILES =

argtypesdir = $(datadir)/pygtk/2.0/argtypes
argtypes_PYTHON = matecomponent-arg-types.py

pymatecomponentexec_LTLIBRARIES =

if BUILD_MATECOMPONENT_ACTIVATION
pymatecomponentexec_LTLIBRARIES += activation.la
endif
activation_la_CFLAGS = $(MATECOMPONENT_ACTIVATION_CFLAGS)
activation_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initactivation
activation_la_LIBADD = $(MATECOMPONENT_ACTIVATION_LIBS) $(PYTHON_LIBS)
activation_la_SOURCES = activationmodule.c

if BUILD_MATECOMPONENT
pymatecomponentexec_LTLIBRARIES += _matecomponent.la
endif
_matecomponent_la_CFLAGS = $(MATECOMPONENT_CFLAGS)
_matecomponent_la_LDFLAGS = $(common_ldflags) -export-symbols-regex init_matecomponent
_matecomponent_la_LIBADD = $(MATECOMPONENT_LIBS) $(PYTHON_LIBS)
_matecomponent_la_SOURCES = matecomponentmodule.c matecomponent-arg.c pymatecomponent.h pymatecomponent.c
nodist__matecomponent_la_SOURCES = matecomponent.c
CLEANFILES += matecomponent.c
EXTRA_DIST += matecomponent.override
matecomponent.c: matecomponent.defs matecomponent-types.defs matecomponent.override

if BUILD_MATECOMPONENTUI
pymatecomponentexec_LTLIBRARIES += ui.la
endif
ui_la_CFLAGS = $(MATECOMPONENTUI_CFLAGS)
ui_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initui
ui_la_LIBADD = $(MATECOMPONENTUI_LIBS) $(PYTHON_LIBS)
ui_la_SOURCES = matecomponentuimodule.c
nodist_ui_la_SOURCES = matecomponentui.c
CLEANFILES += matecomponentui.c
EXTRA_DIST += matecomponentui.override
matecomponentui.c: matecomponentui.defs matecomponentui-types.defs matecomponent-types.defs matecomponentui.override

.defs.c:
	(cd $(srcdir) \
	 && $(PYMATE_CODEGEN) \
	    --py_ssize_t-clean \
	    --load-types matecomponent-arg-types.py \
	    --register $(PYGTK_DEFSDIR)/pango-types.defs \
	    --register $(PYGTK_DEFSDIR)/gdk-types.defs \
	    --register $(PYGTK_DEFSDIR)/gtk-types.defs \
	    --register ../matecanvas/canvas.defs \
	    --register ../mate/mate-types.defs \
	    --register matecomponent-types.defs \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

EXTRA_DIST += wscript
