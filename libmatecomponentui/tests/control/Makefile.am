noinst_PROGRAMS =		\
	test-control

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	-DMATECOMPONENT_UIDIR=\""$(datadir)/mate/ui/"\"	\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(MATECOMPONENTUI_CFLAGS)

LDADD =	$(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)

test_control_SOURCES = test-control.c

