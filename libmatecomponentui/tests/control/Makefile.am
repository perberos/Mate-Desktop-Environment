noinst_PROGRAMS =		\
	mate-test-control

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	-DMATECOMPONENT_UIDIR=\""$(datadir)/mate/ui/"\"	\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(MATECOMPONENTUI_CFLAGS)

LDADD =	$(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)

mate_test_control_SOURCES = mate-test-control.c

