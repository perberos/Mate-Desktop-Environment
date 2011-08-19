
INCLUDES = \
	-I$(top_srcdir) 				\
	-I$(top_builddir) 				\
	$(WARN_CFLAGS)					\
	$(LIBMATECOMPONENT_CFLAGS)				\
	-DPREFIX=\""$(prefix)"\"			\
        -DMATECOMPONENT_LIBDIR=\""$(libdir)"\" 		\
        -DMATECOMPONENT_DATADIR=\""$(datadir)"\" 		\
        -DMATECOMPONENT_PIXMAPDIR=\""$(datadir)/pixmaps"\"	\
        -DMATECOMPONENT_BINDIR=\""$(bindir)"\" 		\
        -DMATECOMPONENT_LOCALSTATEDIR=\""$(localstatedir)"\" 	\
        -DMATECOMPONENT_LOCALEDIR=\""$(datadir)/locale"\" 	\
	-DMATECOMPONENT_EXPLICIT_TRANSLATION_DOMAIN="GETTEXT_PACKAGE"	\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"

AM_LDFLAGS=$(LIBMATECOMPONENT_LT_VERSION_INFO)

CORBA_SOURCE =			\
	MateComponent.h		\
	MateComponent-common.c		\
	MateComponent-stubs.c		\
	MateComponent-skels.c

idls =							\
	$(top_srcdir)/idl/MateComponent.idl			\
	$(top_srcdir)/idl/MateComponent_Canvas.idl		\
	$(top_srcdir)/idl/MateComponent_Context.idl		\
	$(top_srcdir)/idl/MateComponent_Control.idl		\
	$(top_srcdir)/idl/MateComponent_Embeddable.idl		\
	$(top_srcdir)/idl/MateComponent_Gdk.idl		\
	$(top_srcdir)/idl/MateComponent_ItemContainer.idl	\
	$(top_srcdir)/idl/MateComponent_Listener.idl		\
	$(top_srcdir)/idl/MateComponent_Moniker.idl		\
	$(top_srcdir)/idl/MateComponent_Persist.idl		\
	$(top_srcdir)/idl/MateComponent_Print.idl		\
	$(top_srcdir)/idl/MateComponent_Property.idl		\
	$(top_srcdir)/idl/MateComponent_Storage.idl		\
	$(top_srcdir)/idl/MateComponent_UI.idl			\
	$(top_srcdir)/idl/MateComponent_Zoomable.idl		\
	$(top_srcdir)/idl/MateComponent_Exception.idl		\
	$(top_srcdir)/idl/MateComponent_Application.idl	\
	$(top_srcdir)/idl/MateComponent_Clipboard.idl

idl_flags = -I$(top_srcdir)/idl -D__MateComponent_COMPILATION

$(CORBA_SOURCE): $(idls) $(MATECORBA_IDL)
	$(AM_V_GEN) $(MATECORBA_IDL) $(idl_flags) $(top_srcdir)/idl/MateComponent.idl

lib_LTLIBRARIES = libmatecomponent-2.la

libmatecomponent_2_la_LIBADD = \
	$(LIBMATECOMPONENT_LIBS) \
	$(top_builddir)/matecomponent-activation/libmatecomponent-activation.la \
	$(INTLLIBS)

libmatecomponent_2_la_LDFLAGS = -no-undefined

libmatecomponentincludedir = $(includedir)/libmatecomponent-2.0/matecomponent

libmatecomponentinclude_HEADERS =		\
	MateComponent.h			\
	matecomponent-arg.h			\
	matecomponent-context.h		\
	matecomponent-event-source.h		\
	matecomponent-exception.h		\
	matecomponent-generic-factory.h	\
	matecomponent-item-container.h		\
	matecomponent-item-handler.h		\
	matecomponent-listener.h		\
	matecomponent-main.h			\
	matecomponent-macros.h			\
	matecomponent-moniker-extender.h	\
	matecomponent-moniker-simple.h		\
	matecomponent-moniker-util.h		\
	matecomponent-moniker.h		\
	matecomponent-object.h			\
	matecomponent-foreign-object.h		\
	matecomponent-persist-file.h		\
	matecomponent-persist-stream.h		\
	matecomponent-persist.h		\
	matecomponent-persist-client.h		\
	matecomponent-property-bag.h		\
	matecomponent-property-bag-client.h	\
	matecomponent-shlib-factory.h		\
	matecomponent-storage.h		\
	matecomponent-stream.h			\
	matecomponent-stream-client.h		\
	matecomponent-stream-memory.h		\
	matecomponent-storage-memory.h		\
	matecomponent-xobject.h		\
	matecomponent-i18n.h			\
	matecomponent-application.h		\
	matecomponent-app-client.h		\
	matecomponent-types.h

noinst_HEADERS =			\
	matecomponent-moniker-context.h	\
	matecomponent-running-context.h	\
	matecomponent-private.h		\
	matecomponent-marshal.h

libmatecomponent_src =				\
	matecomponent-moniker-context.c	\
	matecomponent-arg.c			\
	matecomponent-context.c		\
	matecomponent-event-source.c		\
	matecomponent-exception.c		\
	matecomponent-generic-factory.c	\
	matecomponent-item-container.c		\
	matecomponent-item-handler.c		\
	matecomponent-listener.c		\
	matecomponent-main.c			\
	matecomponent-moniker.c		\
	matecomponent-moniker-extender.c	\
	matecomponent-moniker-simple.c		\
	matecomponent-moniker-util.c		\
	matecomponent-object.c			\
	matecomponent-foreign-object.c		\
	matecomponent-persist-file.c		\
	matecomponent-persist-stream.c		\
	matecomponent-persist.c		\
	matecomponent-persist-client.c		\
	matecomponent-property-bag.c		\
	matecomponent-property-bag-client.c	\
	matecomponent-shlib-factory.c		\
	matecomponent-storage.c		\
	matecomponent-stream-client.c		\
	matecomponent-stream-memory.c		\
	matecomponent-storage-memory.c		\
	matecomponent-running-context.c	\
	matecomponent-application.c		\
	matecomponent-app-client.c		\
	matecomponent-types.c			\
	matecomponent-debug.c			\
	matecomponent-debug.h

$(libmatecomponent_2_la_OBJECTS): MateComponent.h $(marshal_sources)

libmatecomponent_2_la_SOURCES =		\
	$(libmatecomponentinclude_HEADERS)	\
	$(libmatecomponent_src)		\
	$(CORBA_SOURCE)

marshal_sources = \
	matecomponent-marshal.c \
	matecomponent-marshal.h

matecomponent-marshal.h: matecomponent-marshal.list Makefile
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --header --prefix=matecomponent_marshal > $@
matecomponent-marshal.c: matecomponent-marshal.list Makefile
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $< --body --prefix=matecomponent_marshal > $@

EXTRA_DIST = libmatecomponent-2.0.pc.in libmatecomponent-2.0-uninstalled.pc.in matecomponent-marshal.list

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmatecomponent-2.0.pc

BUILT_SOURCES = $(CORBA_SOURCE) $(marshal_sources)
CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)

