SUBDIRS=include orb-core util GIOP poa dynamic

lib_LTLIBRARIES=libMateCORBA-2.la

INCLUDES =					\
	-I.					\
	-I$(srcdir)				\
	-I$(top_srcdir)/include			\
	-I$(top_builddir)/include		\
	-DMATECORBA_VERSION=\"@MATECORBA_VERSION@\"	\
	-DMATECORBA2_INTERNAL_API			\
	$(WARN_CFLAGS)				\
	$(LOCAL_LINC_CFLAGS)			\
	$(MATECORBA_CFLAGS)

libMateCORBA_2_la_SOURCES=matecorba-init.c matecorba-init.h

libMateCORBA_2_la_LDFLAGS =               \
	-version-info $(LT_VERSION)   \
	-no-undefined		      \
	$(MATECORBA_LIBS)

libMateCORBA_2_la_LIBADD= \
	util/liborb-util.la \
	GIOP/libGIOP.la \
	orb-core/liborb-core.la \
	poa/liborb-poa.la \
	dynamic/liborb-dynamic.la \
	../../linc2/src/liblinc.la

inst-lib: all-recursive install-libLTLIBRARIES
