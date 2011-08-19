noinst_PROGRAMS = matecomponent-hello

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	-DMATECOMPONENT_UIDIR=\""$(datadir)/mate/ui/"\"	\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(MATECOMPONENTUI_CFLAGS)				\
	-DHELLO_SRCDIR=\""$(srcdir)"/\"

LDADD =	$(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)

matecomponent_hello_SOURCES =	\
	hello.c

uidir = $(datadir)/mate-2.0/ui
ui_DATA = MateComponent_Sample_Hello.xml

EXTRA_DIST=$(ui_DATA)
