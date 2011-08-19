NULL =

lib_LTLIBRARIES = libMateCORBA-imodule-2.la

INCLUDES =				\
        -I$(top_srcdir)/include		\
        -I$(top_builddir)/include	\
	-DMATECORBA2_INTERNAL_API		\
	$(WARN_CFLAGS)			\
	$(LOCAL_LINC_CFLAGS)		\
	$(MATECORBA_CFLAGS)			\
	$(MATECORBA_IDL_CFLAGS)		\
	$(NULL)

LDADD = $(MATECORBA_LIBS)				\
	$(MATECORBA_IDL_LIBS)			\
	$(top_builddir)/src/orb/libMateCORBA-2.la

imoduleincludedir      = $(includedir)/matecorba-2.0/MateCORBAservices
imoduleinclude_HEADERS = matecorba-imodule.h

libMateCORBA_imodule_2_la_SOURCES =		\
	matecorba-imodule-utils.c		\
	matecorba-imodule-utils.h		\
	matecorba-imodule-libidl-utils.c	\
	matecorba-imodule-libidl-utils.h	\
	matecorba-imodule.c			\
	$(NULL)

libMateCORBA_imodule_2_la_LIBADD = \
	$(top_builddir)/src/orb/libMateCORBA-2.la \
	$(MATECORBA_IDL_LIBS) \
	$(MATECORBA_LIBS)

libMateCORBA_imodule_2_la_LDFLAGS = -no-undefined
