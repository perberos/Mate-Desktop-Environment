if BUILD_PYTHON
INCLUDES =			\
	-I$(top_srcdir)		\
	$(PYTHON_INCLUDES)	\
	$(PYGOBJECT_CFLAGS)	\
	$(GTK_CFLAGS)

pyunstabledir = $(pyexecdir)/mateweather/I_KNOW_THIS_IS_UNSTABLE
pyunstable_LTLIBRARIES = mateweather.la
mateweather_la_LDFLAGS = -module -avoid-version -export-symbols-regex initmateweather
mateweather_la_LIBADD = ../libmateweather/libmateweather.la
mateweather_la_SOURCES = mateweathermodule.c
nodist_mateweather_la_SOURCES = mateweather.c
mateweather.c: mateweather.defs mateweather.override

.defs.c:
	$(AM_V_GEN)(cd $(srcdir)\
	 && pygobject-codegen-2.0 \
		--register $(PYGTK_DEFS) \
		--override $*.override \
		--prefix py$* $*.defs) > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

install-exec-hook:
	$(MKDIR_P) "$(DESTDIR)$(pyunstabledir)"
	$(INSTALL_DATA) $(srcdir)/bad__init__.py "$(DESTDIR)$(pyexecdir)/mateweather/__init__.py"
	touch $(DESTDIR)$(pyunstabledir)/__init__.py
	chmod a+x $(DESTDIR)$(pyunstabledir)/__init__.py

endif

EXTRA_DIST = mateweather.defs mateweather.override bad__init__.py demo.py

CLEANFILES = mateweather.c

-include $(top_srcdir)/git.mk
