INCLUDES = $(PYTHON_INCLUDES) $(PYGTK_CFLAGS)

defsdir = $(datadir)/pygtk/2.0/defs

defs_DATA = mateconf.defs

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

pkgpythondir = $(pyexecdir)/gtk-2.0
pkgpyexecdir = $(pyexecdir)/gtk-2.0

# mateconf binding

pkgpyexec_LTLIBRARIES = mateconf.la
mateconf_la_CFLAGS = $(MATECONF_CFLAGS)
mateconf_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initmateconf
mateconf_la_LIBADD = $(MATECONF_LIBS) $(PYTHON_LIBS)
mateconf_la_SOURCES = \
	mateconfmodule.c \
	mateconf-fixes.c \
	mateconf-fixes.h \
	mateconf-types.c \
	mateconf-types.h
nodist_mateconf_la_SOURCES = mateconf.c
CLEANFILES = mateconf.c
EXTRA_DIST = mateconf.defs mateconf.override  
mateconf.c: mateconf.defs mateconf.override mateconf-arg-types.py

argtypesdir = $(datadir)/pygtk/2.0/argtypes
argtypes_PYTHON = mateconf-arg-types.py

.defs.c:
	(cd $(srcdir) \
	 && $(PYMATE_CODEGEN) \
	    --py_ssize_t-clean \
	    --load-types mateconf-arg-types.py \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c


EXTRA_DIST += wscript

