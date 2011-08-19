SUBDIRS = test-activation models

check_PROGRAMS =		\
	test-thread		\
	test-moniker		\
	test-properties-server	\
	test-properties-client	\
	test-event-source	\
	test-object		\
	test-stream-mem		\
	test-storage-mem	\
	test-main-loop		\
	generic-factory		\
	test-uniqapp

#check_PROGRAMS += \
#        test-generic-factory

INCLUDES =	\
        -DMATELOCALEDIR=\""$(datadir)/locale"\"        \
	-I$(top_srcdir) 				\
	$(WARN_CFLAGS)					\
	$(LIBMATECOMPONENT_CFLAGS)				\
	$(DISABLE_DEPRECATED_CFLAGS)

LDADD =	$(top_builddir)/matecomponent/libmatecomponent-2.la $(LIBMATECOMPONENT_LIBS)

test_properties_server_SOURCES =			\
	test-properties-server.c

test_properties_client_SOURCES =			\
	test-properties-client.c

test_thread_SOURCES = test-thread.c

test_moniker_SOURCES = test-moniker.c

test_object_SOURCES = test-object.c

test_event_source_SOURCES = test-event-source.c

test_stream_mem_SOURCES = test-stream-mem.c

test_storage_mem_SOURCES = test-storage-mem.c

test_uniqapp_SOURCES = test-uniqapp.c

test_main_loop_SOURCES = test-main-loop.c

TESTS_ENVIRONMENT =					\
	LD_LIBRARY_PATH="$(top_builddir)/matecomponent/.libs:$$LD_LIBRARY_PATH"	\
	PATH="$(top_builddir)/matecomponent/.libs:$$PATH"	\
	MATECOMPONENT_PLUGIN_PATH="$(top_builddir)/storage-modules/.libs:$$MATECOMPONENT_PLUGIN_PATH"\
	MATECOMPONENT_ACTIVATION_PATH="$(srcdir)$(SEARCHPATHSEP)$$MATECOMPONENT_ACTIVATION_PATH"\
	MODELS_DIR="$(srcdir)/models"

TESTS = test-moniker$(EXEEXT) test-event-source$(EXEEXT) test-object$(EXEEXT) \
	test-stream-mem$(EXEEXT) $(srcdir)/test-properties.sh test-storage-mem$(EXEEXT) \
	test-main-loop$(EXEEXT)

# TESTS += test-generic-factory.sh

check_SCRIPTS = test-properties.sh test-generic-factory.sh
EXTRA_DIST=$(check_SCRIPTS) Test_Generic_Factory.server test-generic-factory.sh
