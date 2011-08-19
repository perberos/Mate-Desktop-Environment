tests =  \
	testvfs.py testmatecomponent.py testmate.py

PYTHONPATH = $(top_builddir):$(top_builddir)/matecanvas:$(top_builddir):$(top_builddir)/matecomponent:$(top_builddir)/mateconf:$(shell echo $$PYTHONPATH)

check-local:
	@if ! test -e $(top_builddir)/mate/__init__.py; then cp $(top_srcdir)/mate/__init__.py $(top_builddir)/mate/__init__.py; fi
	@if ! test -e $(top_builddir)/matevfs/__init__.py; then cp $(top_srcdir)/matevfs/__init__.py $(top_builddir)/matevfs/__init__.py; fi
	@if ! test -e $(top_builddir)/matecomponent/__init__.py; then cp $(top_srcdir)/matecomponent/__init__.py $(top_builddir)/matecomponent/__init__.py; fi
	@PYTHONPATH=$(PYTHONPATH) DIST_CHECK=1 $(PYTHON) -W error $(srcdir)/runtests.py

	@if test "$(top_builddir)" != "$(top_srcdir)"; then\
		rm -f $(top_builddir)/mate/__init__.py*;\
		rm -f $(top_builddir)/matevfs/__init__.py*;\
		rm -f $(top_builddir)/matecomponent/__init__.py*;\
	fi
	@rm -fr *.pyc 

EXTRA_DIST = $(tests) common.py runtests.py ltihooks.py
