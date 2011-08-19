invest-applet: invest-applet.py
	sed -e "s|\@PYTHONDIR\@|$(pythondir)/|" $< > $@

bin_SCRIPTS = invest-chart

libexec_SCRIPTS = invest-applet

investdir = $(pythondir)/invest
invest_PYTHON = \
	__init__.py \
	about.py \
	help.py \
	applet.py \
	chart.py \
	currencies.py \
	widgets.py \
	quotes.py \
	networkmanager.py \
	preferences.py
nodist_invest_PYTHON = \
	defs.py

BUILT_SOURCES = \
	invest-applet

CLEANFILES = \
	$(BUILT_SOURCES)

DISTCLEANFILES = \
	defs.py \
	$(CLEANFILES)

EXTRA_DIST = \
	defs.py.in \
	invest-applet.py \
	invest-chart \
	test.py

#TESTS = test.py

-include $(top_srcdir)/git.mk
