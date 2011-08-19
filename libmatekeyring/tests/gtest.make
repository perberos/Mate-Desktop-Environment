
# The following need to be declared before this file is included:
#   TEST_AUTO     A list of C files with tests
#   TEST_LIBS     Libraries to link the tests to
#   TEST_FLAGS    Flags for the tests

# ------------------------------------------------------------------------------

INCLUDES= \
	-I$(top_srcdir) \
	-I$(top_builddir) \
	-I$(srcdir)/.. \
	-I$(srcdir)/../.. \
	$(GLIB_CFLAGS)

LIBS = \
	$(GLIB_LIBS)

noinst_PROGRAMS= \
	run-auto-test

run-auto-test.h: $(TEST_AUTO) Makefile.am $(top_srcdir)/tests/prep-gtest.sh
	sh $(top_srcdir)/tests/prep-gtest.sh -b run-auto-test $(TEST_AUTO)

run-auto-test.c: run-auto-test.h

run_auto_test_SOURCES = \
	run-auto-test.c run-auto-test.h \
	$(TEST_AUTO)

run_auto_test_LDADD = \
	$(TEST_LIBS) \
	$(LIBRARY_LIBS)

run_auto_test_CFLAGS = \
	$(TEST_FLAGS)

BUILT_SOURCES = \
	run-auto-test.c \
	run-auto-test.h

# ------------------------------------------------------------------------------
# Run the tests

test-auto: $(noinst_PROGRAMS)
	gtester --verbose -k -m=slow ./run-auto-test

check-am: $(noinst_PROGRAMS)
	TEST_DATA=$(srcdir)/test-data gtester -m=slow ./run-auto-test
