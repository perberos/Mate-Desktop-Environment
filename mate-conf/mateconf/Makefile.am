NULL=

MATECONFD_BINARY_NAME=mateconfd-2

INCLUDES=								\
	-I$(top_srcdir)							\
	-I$(top_builddir)						\
	$(DEPENDENT_WITH_XML_AND_GTK_CFLAGS) 				\
	-DG_LOG_DOMAIN=\"MateConf\"					\
	-DPREFIX=\""$(prefix)"\"					\
	-DMATECONF_LOCALE_DIR=\""$(datadir)/locale"\"			\
        -DMATECONF_SRCDIR=\""$(absolute_top_srcdir)"\"			\
	-DMATECONF_CONFDIR=\""$(sysmateconfdir)/$(MAJOR_VERSION)"\"		\
	-DMATECONF_ETCDIR=\""$(sysmateconfdir)"\"				\
	-DMATECONF_BINDIR=\""$(bindir)"\"					\
	-DMATECONF_SERVERDIR=\""$(libexecdir)"\"           \
	-DMATECONF_BUILDDIR=\""$(top_builddir)"\"				\
	-DMATECONF_BACKEND_DIR=\""$(pkglibdir)/$(MAJOR_VERSION)"\"		\
	-DVERSION=\""$(VERSION)"\"					\
	-DMATECONF_ENABLE_INTERNALS=1					\
	-DMATECONFD=\""$(MATECONFD_BINARY_NAME)$(EXEEXT)"\"			\
	$(NULL)

EFENCE=

if GTK
SANITY_CHECK=mateconf-sanity-check-2
else
SANITY_CHECK=
endif

lib_LTLIBRARIES = libmateconf-2.la

bin_PROGRAMS = mateconftool-2 
libexec_PROGRAMS = mateconfd-2 $(SANITY_CHECK)

CORBA_SOURCECODE = MateConfX-common.c MateConfX-skels.c MateConfX-stubs.c MateConfX.h

BUILT_SOURCES = 			\
	$(CORBA_SOURCECODE)		\
	mateconfmarshal.h			\
	mateconfmarshal.c

$(libmateconf_2_la_OBJECTS) $(mateconftool_2_OBJECTS) $(mateconfd_2_OBJECTS): $(built_sourcecode)

CLEANFILES = $(BUILT_SOURCES) $(service_DATA)

DISTCLEANFILES = default.path

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)

mateconf_headers = 		\
	mateconf.h			\
	mateconf-changeset.h	\
	mateconf-listeners.h	\
	mateconf-schema.h		\
	mateconf-value.h		\
	mateconf-error.h		\
	mateconf-engine.h		\
	mateconf-client.h

mateconfincludedir = $(includedir)/mateconf/$(MAJOR_VERSION)/mateconf
mateconfinclude_HEADERS = 		\
	$(mateconf_headers) 	\
	mateconf-enum-types.h

## This is broken I know, I'm going to break the server/client up and have a library later

mateconfd_2_SOURCES = \
	mateconf-database.h	\
	mateconf-database.c	\
	mateconf-sources.h		\
	mateconfd.h		\
	mateconfd.c

if OS_WIN32
mateconfd_2_LDFLAGS = -mwindows
endif

mateconfd_2_CFLAGS = $(INCLUDES) $(DEPENDENT_DBUS_CFLAGS)

mateconfd_2_LDADD = libmateconf-$(MAJOR_VERSION).la $(EFENCE) $(INTLLIBS) $(DEPENDENT_LIBS) $(DEPENDENT_DBUS_LIBS)

mateconftool_2_SOURCES = \
	mateconftool.c

mateconftool_2_LDADD = libmateconf-$(MAJOR_VERSION).la $(EFENCE) $(INTLLIBS) $(DEPENDENT_WITH_XML_LIBS)

mateconf_sanity_check_2_SOURCES = \
	mateconf-sanity-check.c

mateconf_sanity_check_2_LDADD = libmateconf-$(MAJOR_VERSION).la $(EFENCE) $(INTLLIBS) $(DEPENDENT_WITH_XML_AND_GTK_LIBS)

$(CORBA_SOURCECODE): $(srcdir)/MateConfX.idl $(MATECORBA_IDL)
	$(MATECORBA_IDL) $(srcdir)/MateConfX.idl

if OS_WIN32
WIN32_SOURCECODE = dllmain.c
endif

libmateconf_2_la_SOURCES = \
	mateconf-internals.c	\
	mateconf-internals.h	\
	mateconf-backend.h		\
	mateconf-backend.c		\
	mateconf-changeset.c	\
	mateconf-error.c		\
	mateconf-listeners.c	\
	mateconf-locale.h  	\
	mateconf-locale.c  	\
	mateconf-schema.c		\
	mateconf-sources.c		\
	mateconf-value.c		\
	mateconf.c			\
	mateconf-client.c		\
	mateconf-enum-types.c	\
	$(CORBA_SOURCECODE)	\
	$(WIN32_SOURCECODE)

libmateconf_2_la_LDFLAGS = -version-info $(MATECONF_CURRENT):$(MATECONF_REVISION):$(MATECONF_AGE) -no-undefined

libmateconf_2_la_LIBADD = $(INTLLIBS) $(DEPENDENT_LIBS)

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --warn-all --add-include-path=$(srcdir)
INTROSPECTION_COMPILER_ARGS= --includedir=$(srcdir)

if HAVE_INTROSPECTION
introspection_files =				\
	$(mateconfinclude_HEADERS)			\
	$(filter %.c,$(libmateconf_2_la_SOURCES))
MateConf-2.0.gir: libmateconf-2.la Makefile
MateConf_2_0_gir_INCLUDES = GObject-2.0
MateConf_2_0_gir_CFLAGS = \
	$(INCLUDES)
MateConf_2_0_gir_LIBS = libmateconf-2.la
MateConf_2_0_gir_FILES = $(addprefix $(srcdir)/, $(introspection_files))
MateConf_2_0_gir_SCANNERFLAGS = --identifier-prefix=MateConf --symbol-prefix=mateconf
INTROSPECTION_GIRS += MateConf-2.0.gir

girdir = $(datadir)/gir-1.0/
gir_DATA = $(INTROSPECTION_GIRS)

typelibsdir = $(libdir)/girepository-1.0/
typelibs_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelibs_DATA)
endif

EXTRA_DIST=MateConfX.idl default.path.in org.mate.MateConf.service.in mateconfmarshal.list regenerate-enum-header.sh regenerate-enum-footer.sh

default.path: $(srcdir)/default.path.in
	sed -e 's,[@]sysmateconfdir[@],$(sysmateconfdir),g' \
		<$(srcdir)/default.path.in >default.path
org.mate.MateConf.service:  $(srcdir)/org.mate.MateConf.service.in
	sed -e 's,[@]libexecdir[@],$(libexecdir),g' \
		<$(srcdir)/org.mate.MateConf.service.in >org.mate.MateConf.service

servicedir = $(datadir)/dbus-1/services
service_DATA = org.mate.MateConf.service

install-data-local: default.path
	$(mkinstalldirs) $(DESTDIR)$(sysmateconfdir)/$(MAJOR_VERSION)
	if test -f $(DESTDIR)$(sysmateconfdir)/$(MAJOR_VERSION)/path; then				\
		echo "Existing path configuration file not changed";				\
	else 											\
		$(INSTALL_DATA) default.path $(DESTDIR)$(sysmateconfdir)/$(MAJOR_VERSION)/path;	\
	fi

uninstall-local:
	rm -f $(DESTDIR)$(sysmateconfdir)/$(MAJOR_VERSION)/path

mateconfmarshal.h: mateconfmarshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=mateconf_marshal > $@

mateconfmarshal.c: mateconfmarshal.list $(GLIB_GENMARSHAL)
	echo "#include \"mateconfmarshal.h\"" > $@ && \
	$(GLIB_GENMARSHAL) $< --body --prefix=mateconf_marshal >> $@

regenerate-built-sources:
	MATECONF_SRCDIR=$(srcdir) $(srcdir)/regenerate-enum-header.sh $(mateconf_headers)
	MATECONF_SRCDIR=$(srcdir) $(srcdir)/regenerate-enum-footer.sh $(mateconf_headers)
