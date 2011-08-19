NULL=

idldir = $(datadir)/idl/matecomponent-2.0

idl_DATA =				\
	MateComponent.idl			\
	MateComponent_Canvas.idl		\
	MateComponent_Context.idl		\
	MateComponent_Control.idl		\
	MateComponent_Embeddable.idl		\
	MateComponent_Gdk.idl			\
	MateComponent_ItemContainer.idl	\
	MateComponent_Listener.idl		\
	MateComponent_Moniker.idl		\
	MateComponent_Persist.idl		\
	MateComponent_Print.idl		\
	MateComponent_Property.idl		\
	MateComponent_Storage.idl		\
	MateComponent_UI.idl			\
	MateComponent_Zoomable.idl		\
	MateComponent_Exception.idl		\
	MateComponent_Clipboard.idl		\
	MateComponent_Application.idl		\
	$(NULL)

activation_idldir=$(datadir)/idl/matecomponent-activation-2.0
activation_idl_DATA =                   \
	MateComponent_Unknown.idl              \
	MateComponent_GenericFactory.idl       \
	MateComponent_Activation_types.idl     \
	MateComponent_ObjectDirectory.idl      \
	MateComponent_ActivationContext.idl

EXTRA_DIST = $(idl_DATA) $(activation_idl_DATA)

matecorbatypelib_LTLIBRARIES = MateComponent_module.la
matecorbatypelibdir = $(libdir)/matecorba-2.0

IMODULE_SOURCE =	\
	MateComponent.h	\
	MateComponent-common.c	\
	MateComponent-imodule.c

idl_flags = \
	-I$(top_srcdir)/idl \
	-D__MateComponent_COMPILATION \
	-D__MateComponent_Unknown_COMPILATION \
	-D__MateComponent_GenericFactory_COMPILATION \
	-D__MateComponent_Activation_types_COMPILATION

INCLUDES =  \
	-I. \
	$(LIBMATECOMPONENT_CFLAGS) \
	$(WARN_CFLAGS)

MateComponent_module_la_SOURCES = \
	MateComponent-imodule.c

MateComponent_module_la_LIBADD = \
	$(LIBMATECOMPONENT_LIBS)

MateComponent_module_la_LDFLAGS = \
	-module -avoid-version -no-undefined

$(IMODULE_SOURCE): $(idl_DATA) $(MATECORBA_IDL)
	$(AM_V_GEN) $(MATECORBA_IDL) $(idl_flags) --imodule $(top_srcdir)/idl/MateComponent.idl

BUILT_SOURCES = $(IMODULE_SOURCE)
CLEANFILES = $(BUILT_SOURCES)

dist-hook:
	cd $(distdir); rm -f $(BUILT_SOURCES)
