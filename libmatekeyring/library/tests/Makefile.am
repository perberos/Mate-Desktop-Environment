
TEST_AUTO = \
	test-memory.c \
	test-keyrings.c \
	test-other.c \
	test-daemon-setup.c

TEST_LIBS = \
	$(top_builddir)/egg/libegg.la \
	$(top_builddir)/library/libmate-keyring.la

TEST_FLAGS = \
	-DTEST_WITH_DAEMON

include $(top_srcdir)/tests/gtest.make

# ------------------------------------------------------------------

noinst_PROGRAMS += \
	test-prompting

test_prompting_SOURCES = \
	test-prompting.c

test_prompting_CFLAGS = \
	$(GLIB_CFLAGS)

test_prompting_LDADD = \
	$(top_builddir)/library/libmate-keyring.la \
	$(GLIB_LIBS)
