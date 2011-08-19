noinst_PROGRAMS = matecomponent-sample-container

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(MATECOMPONENTUI_CFLAGS)

matecomponent_sample_container_SOURCES =			\
	matecomponent-sample-container.c

matecomponent_sample_container_LDADD =			\
	$(top_builddir)/matecomponent/libmatecomponentui-2.la \
	$(MATECOMPONENTUI_LIBS)			\
	$(INTLLIBS)

uidir = $(datadir)/mate-2.0/ui
ui_DATA = MateComponent_Sample_Container-ui.xml

EXTRA_DIST = $(ui_DATA)