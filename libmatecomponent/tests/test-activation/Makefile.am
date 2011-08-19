check_PROGRAMS=			\
	matecomponent-activation-test		\
	matecomponent-activation-test-async	\
	matecomponent-activation-env-test	\
	matecomponent-activation-empty-server

noinst_LTLIBRARIES=libtest-plugin.la

bin_PROGRAMS=matecomponent-activation-run-query

matecomponent_activation_test_async_SOURCES=   \
	matecomponent-activation-test-async.c

matecomponent_activation_test_SOURCES=		\
	matecomponent-activation-test.c	\
	empty-common.c			\
	empty.h				\
	empty-stubs.c 

matecomponent_activation_test_LDADD=libtest-plugin.la ../../matecomponent/libmatecomponent-2.la
matecomponent_activation_env_test_LDADD=../../matecomponent/libmatecomponent-2.la


# Horrible, digusting and atrocious hack to
# get libtool to build a noinst shared lib.
libtest_plugin_la_LDFLAGS=					\
	-rpath $(shell (cd $(top_builddir);pwd))/test/.libs 	\
	-module -avoid-version -no-undefined

libtest_plugin_la_LIBADD=					\
	../../matecomponent-activation/libmatecomponent-activation.la		\
	$(LIBMATECOMPONENT_LIBS)

libtest_plugin_la_SOURCES=		\
	plugin.h			\
	plugin-common.c			\
	plugin-skels.c			\
	plugin-stubs.c			\
	matecomponent-activation-testplugin.c

matecomponent_activation_run_query_SOURCES=	\
	matecomponent-activation-run-query.c 	\
	empty-common.c			\
	empty.h				\
	empty-stubs.c

matecomponent_activation_empty_server_SOURCES=	\
	empty-server.c			\
	empty.h				\
	empty-common.c			\
	empty-skels.c

BUILT_SOURCES=				\
	empty.h				\
	empty-common.c			\
	empty-stubs.c			\
	empty-skels.c			\
	plugin.h			\
	plugin-stubs.c			\
	plugin-skels.c			\
	plugin-common.c

empty.h empty-common.c empty-stubs.c empty-skels.c: empty_built
plugin.h plugin-common.c plugin-stubs.c plugin-skels.c: plugin_built

empty_built: empty.idl $(MATECORBA_IDL)
	$(AM_V_GEN) $(MATECORBA_IDL) $(srcdir)/empty.idl
	@touch empty_built

plugin_built: plugin.idl $(MATECORBA_IDL)
	$(AM_V_GEN) $(MATECORBA_IDL) $(srcdir)/plugin.idl
	@touch plugin_built

CLEANFILES=empty_built plugin_built $(BUILT_SOURCES)

INCLUDES= \
	-I$(top_srcdir)			\
	-I$(top_builddir)		\
	$(WARN_CFLAGS)			\
	$(DISABLE_DEPRECATED_CFLAGS) $(LIBMATECOMPONENT_CFLAGS)

LDADD = ../../matecomponent-activation/libmatecomponent-activation.la \
	$(LIBMATECOMPONENT_LIBS)

serverfiles=empty.server broken.server plugin.server

noinst_DATA=$(serverfiles)

EXTRA_DIST= test.sh empty.idl plugin.idl $(serverfiles)

# TESTS= test.sh matecomponent-activation-test-async
TESTS= test.sh

dist-hook:
	cd $(distdir); rm -f $(CLEANFILES)

TESTS_ENVIRONMENT = \
	MATECOMPONENT_ACTIVATION_DEBUG=1 \
	MATECOMPONENT_ACTIVATION_PATH="$(srcdir)$(SEARCHPATHSEP)$(top_builddir)/activation-server$(SEARCHPATHSEP)$$MATECOMPONENT_ACTIVATION_PATH"
