NULL =

# FIXMEchpe: keep this in sync with gucharmap/Makefile.am !
srcdirheaders = \
	gucharmap-block-chapters-model.h \
	gucharmap-block-codepoint-list.h \
	gucharmap-chapters-model.h \
	gucharmap-chapters-view.h \
	gucharmap-charmap.h \
	gucharmap-chartable.h \
	gucharmap-codepoint-list.h \
	gucharmap-script-chapters-model.h \
	gucharmap-script-codepoint-list.h \
	gucharmap-unicode-info.h \
	$(NULL)
builddirheaders = \
	gucharmap.h \
	gucharmap-type-builtins.h \
	$(NULL)

defsdir = $(datadir)/pygtk/2.0/defs
defs_DATA = gucharmap.defs

pkgpyexecdir = $(pyexecdir)/gtk-2.0
pkgpyexec_LTLIBRARIES = gucharmap.la

BUILT_SOURCES = \
	gucharmap.c \
	$(NULL)

gucharmap_la_SOURCES = \
	gucharmap.c \
	gucharmapmodule.c \
	$(NULL)

gucharmap_la_CPPFLAGS = \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	$(PYTHON_INCLUDES) \
	$(DISABLE_DEPRECATED) \
	$(AM_CPPFLAGS)

gucharmap_la_CFLAGS = \
	$(NO_STRICT_ALIASING_CFLAGS) \
	$(PYGTK_CFLAGS) \
	$(GTK_CFLAGS) \
	$(WARN_CFLAGS) \
	$(AM_CFLAGS)

gucharmap_la_LIBADD = \
	$(top_builddir)/gucharmap/libgucharmap.la \
	$(PYGTK_LIBS) \
        $(PYTHON_LIB_LOC) \
	$(PYTHON_LIBS) \
        $(PYTHON_EXTRA_LIBS)

gucharmap_la_LDFLAGS = \
	-module \
	-avoid-version	\
	-export-symbols-regex initgucharmap \
	$(AM_LDFLAGS)

CLEANFILES = \
	$(BUILT_SOURCES) \
	$(NULL)

EXTRA_DIST = \
	gucharmap.defs \
	gucharmap.override \
	$(NULL)

gucharmap.c: gucharmap.defs gucharmap.override

.defs.c:
	$(AM_V_GEN) (cd $(srcdir)\
	 && $(PYGTK_CODEGEN) \
	    --override $*.override \
	    --prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

regenerate-python-binding:
	$(AM_V_GEN) $(PYGTK_H2DEF) \
		$(sort $(addprefix $(top_srcdir)/gucharmap/,$(srcdirheaders)) \
		       $(addprefix $(top_builddir)/gucharmap/,$(builddirheaders))) \
		> gucharmap.defs.new

-include $(top_srcdir)/git.mk
