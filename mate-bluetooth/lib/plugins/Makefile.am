plugindir = $(libdir)/mate-bluetooth/plugins/

INCLUDES =                                      \
        -DDATADIR=\"$(datadir)\"                \
        -DICONDIR=\"$(icondir)\"                \
	-DLOCALEDIR="\"$(datadir)/locale\""	\
        -I$(top_srcdir)/lib			\
        -I$(top_builddir)			\
	$(PLUGINS_CFLAGS)			\
	$(LIBMATEBT_CFLAGS)			\
	$(GEOCLUE_CFLAGS)			\
	$(WARN_CFLAGS)

plugin_LTLIBRARIES = libgbtgeoclue.la

libgbtgeoclue_la_SOURCES = geoclue.c
libgbtgeoclue_la_LDFLAGS = -module -avoid-version
libgbtgeoclue_la_LIBADD = $(PLUGINS_LIBS) $(GEOCLUE_LIBS)

testplugin_LTLIBRARIES = libgbttest.la

testplugindir = $(libdir)/mate-bluetooth/plugins/

libgbttest_la_SOURCES = test.c
libgbttest_la_LDFLAGS = -module -avoid-version
libgbttest_la_LIBADD = $(PLUGINS_LIBS)

# override to _not_ install the test plugins
install-testpluginLTLIBRARIES:

