INCLUDES= -I$(top_srcdir) -I$(top_builddir) -I$(top_builddir)/mateconf \
	$(DEPENDENT_WITH_XML_CFLAGS) \
	-DMATECONF_ENABLE_INTERNALS=1 -DG_LOG_DOMAIN=\"MateConf-Backends\"

backenddir = $(pkglibdir)/$(MAJOR_VERSION)

if LDAP_SUPPORT
EVOLDAP_BACKEND = libmateconfbackend-evoldap.la
endif

backend_LTLIBRARIES = libmateconfbackend-xml.la libmateconfbackend-oldxml.la $(EVOLDAP_BACKEND)

noinst_LTLIBRARIES = libmateconfbackend-oldxml-noinst.la

libmateconfbackend_oldxml_noinst_la_SOURCES = \
	xml-cache.h		\
	xml-cache.c		\
	xml-dir.h		\
	xml-dir.c		\
	xml-entry.h		\
	xml-entry.c		\
	xml-backend.c
libmateconfbackend_oldxml_noinst_la_LIBADD  = $(DEPENDENT_WITH_XML_LIBS) \
	$(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la \
	$(INTLLIBS)

libmateconfbackend_oldxml_la_SOURCES =

libmateconfbackend_oldxml_la_LDFLAGS = -avoid-version -module -no-undefined
libmateconfbackend_oldxml_la_LIBADD  = libmateconfbackend-oldxml-noinst.la

libmateconfbackend_xml_la_SOURCES = 	\
	markup-backend.c		\
	markup-tree.h			\
	markup-tree.c

libmateconfbackend_xml_la_LDFLAGS = -avoid-version -module -no-undefined
libmateconfbackend_xml_la_LIBADD  = $(DEPENDENT_LIBS) $(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la $(INTLLIBS)

noinst_PROGRAMS = xml-test

xml_test_SOURCES= xml-test.c
xml_test_LDADD = \
	$(DEPENDENT_WITH_XML_LIBS) \
	$(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la \
	libmateconfbackend-oldxml-noinst.la

bin_PROGRAMS = mateconf-merge-tree
mateconf_merge_tree_SOURCES = mateconf-merge-tree.c
mateconf_merge_tree_LDADD = $(DEPENDENT_LIBS) $(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la

if LDAP_SUPPORT
libmateconfbackend_evoldap_la_SOURCES = evoldap-backend.c
libmateconfbackend_evoldap_la_LDFLAGS = -avoid-version -module -no-undefined
libmateconfbackend_evoldap_la_LIBADD  = \
	$(DEPENDENT_WITH_XML_LIBS) \
	$(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la \
	$(INTLLIBS) \
	$(LDAP_LIBS)

backendconfdir   = $(sysconfdir)/mateconf/2
backendconf_DATA = evoldap.conf

schemadir   = $(pkgdatadir)/schema
schema_DATA = evoldap.schema
endif

EXTRA_DIST =		\
	README.evoldap	\
	evoldap.conf	\
	evoldap.schema
