INCLUDES =						\
	-DG_LOG_DOMAIN=\"Caja-Python\"	        \
	-DDATADIR=\"$(datadir)\"			\
	-DLIBDIR=\"$(libdir)\"				\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(CAJA_PYTHON_CFLAGS)			\
	-DPYTHON_VERSION=\"$(PYTHON_VERSION)\"		\
	-DPY_LIB_LOC="\"$(PYTHON_LIB_LOC)\""		\
	$(PYTHON_INCLUDES)

caja_extensiondir=$(CAJA_EXTENSION_DIR)

caja_extension_LTLIBRARIES = libcaja-python.la

libcaja_python_la_SOURCES =     	\
	caja-python.c		\
	caja-python.h		\
	caja-python-object.c	\
	caja-python-object.h

libcaja_python_la_LDFLAGS = -module -avoid-version
libcaja_python_la_LIBADD  = $(CAJA_PYTHON_LIBS) $(PYTHON_LIBS)

pyexec_LTLIBRARIES = caja.la
pyexecdir = $(CAJA_LIBDIR)/caja-python

caja_la_LDFLAGS = -module -avoid-version -export-symbols-regex initcaja
caja_la_LIBADD = $(CAJA_PYTHON_LIBS)
caja_la_SOURCES = cajamodule.c
nodist_caja_la_SOURCES = caja.c
caja.c: caja.defs caja.override
CLEANFILES = caja.c
EXTRA_DIST = caja.override caja.defs

.defs.c:
	(cd $(srcdir)\
         && $(PYGTK_CODEGEN) \
	    --register $(PYGTK_DEFSDIR)/pango-types.defs \
            --register $(PYGTK_DEFSDIR)/gdk-types.defs \
            --register $(PYGTK_DEFSDIR)/gtk-types.defs \
            --register $(PYGOBJECT_DEFSDIR)/gio-types.defs \
            --register $(PYGOBJECT_DEFSDIR)/unix-types.defs \
            --override $*.override \
            --prefix py$* $*.defs) > gen-$*.c \
        && cp gen-$*.c $*.c \
        && rm -f gen-$*.c
