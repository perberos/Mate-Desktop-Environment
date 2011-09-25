
NULL =

lib_LTLIBRARIES=libpolkit-gtk-mate-1.la

libpolkit_gtk_mate_1includedir=$(includedir)/polkit-gtk-mate-1/polkitgtkmate

libpolkit_gtk_mate_1include_HEADERS =              				\
	polkitgtkmate.h							\
	polkitgtkmatetypes.h						\
	polkitlockbutton.h						\
	$(NULL)

libpolkit_gtk_mate_1_la_SOURCES =                 	               		\
	polkitgtkmate.h							\
	polkitgtkmatetypes.h						\
	polkitlockbutton.h			polkitlockbutton.c	\
	$(NULL)

libpolkit_gtk_mate_1_la_CPPFLAGS = 				\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	-I$(top_srcdir)/polkitgtkmate			\
	-I$(top_builddir)/polkitgtkmate			\
	-DG_LOG_DOMAIN=\"polkitgtkmate\"			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)					\
	-DPOLKIT_GTK_MATE_COMPILATION			\
	$(NULL)

libpolkit_gtk_mate_1_la_CFLAGS = 				\
	$(POLKIT_GOBJECT_CFLAGS)			\
	$(GTK_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)					\
	$(NULL)

libpolkit_gtk_mate_1_la_LIBADD = 				\
	$(POLKIT_GOBJECT_LIBS)				\
	$(GTK_LIBS)					\
	$(INTLLIBS)					\
	$(NULL)


libpolkit_gtk_mate_1_la_LDFLAGS = -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE) \
		        -export-dynamic -no-undefined -export-symbols-regex '(^polkit_.*)'

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = polkit-gtk-mate-1.pc

noinst_PROGRAMS = example

example_SOURCES = example.c
example_CFLAGS = 					\
	-I$(top_srcdir)					\
	-I$(top_builddir)				\
	$(POLKIT_GOBJECT_CFLAGS)			\
	$(GTK_CFLAGS)					\
	$(WARN_CFLAGS)					\
	$(AM_CFLAGS)					\
	$(NULL)

example_LDADD = 					\
	$(POLKIT_GOBJECT_LIBS)				\
	$(GTK_LIBS)					\
	$(INTLLIBS)					\
	libpolkit-gtk-mate-1.la				\
	$(NULL)

if ENABLE_INTROSPECTION
girdir = $(GIRDIR)
gir_DATA = PolkitGtkMate-1.0.gir

typelibsdir = $(GIRTYPELIBDIR)
typelibs_DATA = PolkitGtkMate-1.0.typelib

PolkitGtkMate-1.0.gir: libpolkit-gtk-mate-1.la $(G_IR_SCANNER) Makefile.am
	$(G_IR_SCANNER) -v 					\
		--namespace PolkitGtkMate 				\
		--strip-prefix=Polkit				\
		--nsversion=1.0 				\
		--include=Gtk-2.0 				\
		--include=Polkit-1.0 				\
		--library=polkit-gtk-mate-1 				\
		--output $@ 					\
		--pkg=polkit-gobject-1				\
		--pkg=gtk+-2.0 					\
		--libtool=$(top_builddir)/libtool		\
                -I$(top_srcdir)		 			\
	        -DPOLKIT_GTK_MATE_COMPILATION                   	\
		$(srcdir)/polkitgtkmate.h 				\
		$(srcdir)/polkitgtkmatetypes.h 			\
		$(srcdir)/polkitlockbutton.h 			\
		$(NULL)

PolkitGtkMate-1.0.typelib: PolkitGtkMate-1.0.gir $(G_IR_COMPILER)
	$(G_IR_COMPILER) $< -o $@

endif # ENABLE_INTROSPECTION

CLEANFILES = $(BUILT_SOURCES) $(pkgconfig_DATA) $(gir_DATA) $(typelibs_DATA)

clean-local :
	rm -f *~ $(BUILT_SOURCES)

