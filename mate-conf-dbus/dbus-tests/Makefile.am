INCLUDES = -I$(top_srcdir) -I$(top_builddir) $(DEPENDENT_CFLAGS) -DG_LOG_DOMAIN=\"MateConf-DBus-Tests\"

noinst_PROGRAMS =		\
	test-stale-value-bug 	\
	test-schema-bug 	\
	test-leaks 

TESTLIBS = $(INTLLIBS) $(DEPENDENT_LIBS) $(top_builddir)/mateconf/libmateconf-$(MAJOR_VERSION).la

test_stale_value_bug_SOURCES=test-stale-value-bug.c
test_stale_value_bug_LDADD = $(TESTLIBS)

test_schema_bug_SOURCES=test-schema-bug.c
test_schema_bug_LDADD = $(TESTLIBS)

test_leaks_SOURCES = test-leaks.c
test_leaks_LDADD = $(TESTLIBS)
