SUBDIRS = control

noinst_PROGRAMS =		\
	test-ui			\
	test-ui-auto		\
	test-toolbar		\
	test-selector		\
	test-filesel		\
	test-focus		\
	test-sensitivity	\
	test-reparent		\
	test-events

bin_PROGRAMS = \
	test-moniker

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

test_ui_SOURCES = test-ui.c
test_ui_auto_SOURCES = test-ui-auto.c
test_focus_SOURCES = test-focus.c
test_toolbar_SOURCES = test-toolbar.c
test_selector_SOURCES =	test-selector.c
test_sensitivity_SOURCES = test-sensitivity.c
test_reparent_SOURCES = test-reparent.c
test_moniker_SOURCES = test-moniker.c
test_filesel_SOURCES = test-filesel.c

TESTS = test-ui-auto
