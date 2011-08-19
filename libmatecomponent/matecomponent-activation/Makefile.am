NULL=

lib_LTLIBRARIES=libmatecomponent-activation.la

INCLUDES=					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-DPREFIX=\""$(prefix)\""		\
	-DSERVER_LIBEXECDIR=\""$(libexecdir)\"" \
	-DSERVERINFODIR=\"$(libdir)/matecomponent/servers\" \
	-DSERVER_CONFDIR=\"$(sysconfdir)\"	\
        -DMATECOMPONENT_ACTIVATION_LOCALEDIR=\""${prefix}/${DATADIRNAME}/locale"\" \
	$(LIBMATECOMPONENT_CFLAGS)			\
	-DVERSION=\"$(VERSION)\"		\
	$(DISABLE_DEPRECATED_CFLAGS)		\
	-DG_LOG_DOMAIN=\"MateComponent-Activation\"	\
	$(NULL)

AM_LDFLAGS=					\
	$(LIBMATECOMPONENT_LIBS)			\
	$(MATECOMPONENT_ACTIVATION_LT_VERSION_INFO)	\
	-no-undefined				\
	$(NULL)

matecomponent_activationincludedir = $(includedir)/matecomponent-activation-2.0/matecomponent-activation

matecomponent_activationinclude_HEADERS=		\
	MateComponent_Unknown.h			\
	MateComponent_GenericFactory.h			\
	MateComponent_Activation_types.h		\
	matecomponent-activation.h			\
	matecomponent-activation-async.h		\
	matecomponent-activation-activate.h		\
	matecomponent-activation-init.h		\
	matecomponent-activation-mime.h		\
	matecomponent-activation-shlib.h		\
	matecomponent-activation-register.h		\
	matecomponent-activation-server-info.h		\
	matecomponent-activation-version.h		\
	$(NULL)


libmatecomponent_activation_la_SOURCES=		\
	$(BUILT_SOURCES)			\
	matecomponent-activation-get-language-list.c	\
	matecomponent-activation-client.c		\
	matecomponent-activation-client.h		\
	matecomponent-activation-id.h			\
	matecomponent-activation-id.c			\
	matecomponent-activation-mime.c		\
	matecomponent-activation-private.h		\
	matecomponent-activation-shlib.c		\
	matecomponent-activation-activate.c		\
	matecomponent-activation-activate-private.h	\
	matecomponent-activation-base-service.h	\
	matecomponent-activation-base-service.c	\
	matecomponent-activation-register.c		\
	matecomponent-activation-server-info.c		\
	matecomponent-activation-fork-server.c		\
	matecomponent-activation-init.c		\
	$(NULL)

libmatecomponent_activation_la_LIBADD = $(INTLLIBS)

IDL_FLAGS = -I$(top_srcdir)/idl
IDL_DIR = $(top_srcdir)/idl/
IDL_FILES = MateComponent_Unknown.idl			\
	    MateComponent_GenericFactory.idl		\
	    MateComponent_Activation_types.idl		\
	    MateComponent_ObjectDirectory.idl		\
	    MateComponent_ActivationContext.idl	\
	    $(NULL)

IDL_HEADER_FILES = $(IDL_FILES:.idl=.h)
IDL_COMMONS = $(IDL_FILES:.idl=-common.c)
IDL_SKELS = $(IDL_FILES:.idl=-skels.c)
IDL_STUBS = $(IDL_FILES:.idl=-stubs.c)

BUILT_SOURCES= $(IDL_HEADER_FILES) $(IDL_COMMONS) $(IDL_SKELS) $(IDL_STUBS) \
	$(NULL)

$(libmatecomponent_activation_la_OBJECTS): $(BUILT_SOURCES)

%.h %-stubs.c %-skels.c %-common.c %-imodule.c %-skelimpl.c: $(IDL_DIR)%.idl $(MATECORBA_IDL)
	@-(rm -f $(*).h $(*)-stubs.c $(*)-skels.c $(*)-common.c $(*)-imodule.c $(*)-skelimpl.c || true) > /dev/null
	$(AM_V_GEN) $(MATECORBA_IDL) $(IDL_FLAGS) "-D__$(*)_COMPILATION" --deps $(*D)/.deps/$(*F).idl.P $<

if MAINTAINER_MODE
DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)
IDL_DEP_FILES = $(IDL_FILES:%.idl=.deps/%.idl.P)
-include $(IDL_DEP_FILES)
distclean-local:
	-rm -rf .deps
endif


CLEANFILES=					\
	$(BUILT_SOURCES)			\
	$(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(CLEANFILES)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = matecomponent-activation-2.0.pc

EXTRA_DIST = $(pkgconfig_DATA) matecomponent-activation-2.0-uninstalled.pc.in AUTHORS
