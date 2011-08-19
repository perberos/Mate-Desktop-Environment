
INCLUDES = -I$(top_srcdir)/src $(PYMATECORBA_CFLAGS) $(PYTHON_INCLUDES)
BUILT_SOURCES = $(idlgen_sources)

idlgen_sources = \
  testcall.h \
  testcall-skels.c \
  testcall-stubs.c \
  testcall-common.c \
  testcall-imodule.c

$(idlgen_sources): testcall.idl
	matecorba-idl-2 --add-imodule $<

noinst_LTLIBRARIES = TestCall_module.la cTestCall.la

TestCall_module_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `pwd`
TestCall_module_la_LIBADD = $(PYMATECORBA_LIBS)
TestCall_module_la_SOURCES = testcall-imodule.c

cTestCall_la_LDFLAGS = -export-dynamic -module -avoid-version -rpath `pwd`
cTestCall_la_LIBADD = $(PYMATECORBA_LIBS)
cTestCall_la_SOURCES = \
  c-impl.c \
  testcall-common.c \
  testcall-stubs.c \
  testcall-skels.c

EXTRA_DIST = testcall.idl test-c-inproc.py

CLEANFILES = $(idlgen_sources)
dist-hook:
	-cd $(distdir) && rm -f $(idlgen_sources)
