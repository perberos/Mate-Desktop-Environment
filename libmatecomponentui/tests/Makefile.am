SUBDIRS = control

noinst_PROGRAMS =		\
	mate-test-ui			\
	mate-test-ui-auto		\
	mate-test-toolbar		\
	mate-test-selector		\
	mate-test-filesel		\
	mate-test-focus		\
	mate-test-sensitivity	\
	mate-test-reparent		\
	mate-test-events

bin_PROGRAMS = \
	mate-test-moniker

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	-DMATECOMPONENT_UI_INTERNAL				\
	-DMATECOMPONENT_UIDIR=\""$(datadir)/mate/ui/"\"	\
	-DMATECOMPONENT_TOPSRCDIR=\""$(top_srcdir)"\"		\
	-DG_LOG_DOMAIN=\"MateComponent\"			\
	-DVERSION=\"$(VERSION)\"			\
	$(WARN_CFLAGS)					\
	$(MATECOMPONENTUI_CFLAGS)

LDADD =	$(top_builddir)/matecomponent/libmatecomponentui-2.la $(MATECOMPONENTUI_LIBS)

mate_test_ui_SOURCES = mate-test-ui.c
mate_test_ui_auto_SOURCES = mate-test-ui-auto.c
mate_test_focus_SOURCES = mate-test-focus.c
mate_test_toolbar_SOURCES = mate-test-toolbar.c
mate_test_selector_SOURCES =	mate-test-selector.c
mate_test_sensitivity_SOURCES = mate-test-sensitivity.c
mate_test_reparent_SOURCES = mate-test-reparent.c
mate_test_moniker_SOURCES = mate-test-moniker.c
mate_test_filesel_SOURCES = mate-test-filesel.c

TESTS = test-ui-auto
