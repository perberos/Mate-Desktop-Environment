appdir = $(pyexecdir)/GMenuSimpleEditor
app_PYTHON = 			\
	__init__.py		\
	main.py			\
	maindialog.py		\
	menutreemodel.py	\
	menufilewriter.py

nodist_app_PYTHON = config.py

config.py: config.py.in
	$(AM_V_GEN)sed								\
		-e s!\@DATADIR\@!$(datadir)!				\
		-e s!\@LOCALEDIR\@!$(prefix)/$(DATADIRNAME)/locale!	\
		-e s!\@PACKAGE_NAME\@!$(PACKAGE_NAME)!			\
		-e s!\@PACKAGE_VERSION\@!$(PACKAGE_VERSION)!		\
		< $< > $@

EXTRA_DIST = config.py.in

CLEANFILES = config.py

-include $(top_srcdir)/git.mk
