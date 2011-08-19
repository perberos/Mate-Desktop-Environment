INCLUDES = $(PYTHON_INCLUDES) $(PYGTK_CFLAGS)

defsdir = $(datadir)/pygtk/2.0/defs
defs_DATA = canvas.defs

EXTRA_DIST = $(defs_DATA)

pkgpythondir = $(pyexecdir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

# libmatecanvas binding
pkgpyexec_LTLIBRARIES = matecanvas.la
matecanvas_la_CFLAGS = $(MATECANVAS_CFLAGS)
matecanvas_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initmatecanvas
matecanvas_la_LIBADD = $(MATECANVAS_LIBS) $(PYTHON_LIBS)
matecanvas_la_SOURCES = canvasmodule.c
nodist_matecanvas_la_SOURCES = canvas.c
CLEANFILES = canvas.c
EXTRA_DIST += canvas.override
canvas.c: canvas.defs canvas.override


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

EXTRA_DIST += wscript
