platform_defines = -DEGG_SM_CLIENT_BACKEND_XSMP
platform_ltlibraries =                   \
	libeggdesktopfile.la             \
	libeggsmclient-mate.la
platform_libs = 			\
	libeggdesktopfile.la		\
	$(SM_LIBS)			\
	$(ICE_LIBS)
platform_sources = eggsmclient-xsmp.c

INCLUDES =                               \
	-DG_LOG_DOMAIN=\""EggSMClient"\" \
	$(platform_defines)              \
	$(EGG_SMCLIENT_CFLAGS)           \
	$(SM_CFLAGS)			 \
	$(ICE_CFLAGS)			 \
	$(WARN_CFLAGS)			 \
	$(DISABLE_DEPRECATED_CFLAGS)

noinst_LTLIBRARIES =                     \
	libeggsmclient.la                \
	$(platform_ltlibraries)

libeggsmclient_la_LIBADD =               \
	$(EGG_SMCLIENT_LIBS)             \
	$(SM_LIBS)			 \
	$(ICE_LIBS)			 \
	$(platform_libs)

libeggsmclient_la_SOURCES =              \
	eggsmclient.c                    \
	eggsmclient.h                    \
	eggsmclient-private.h            \
	$(platform_sources)

libeggsmclient_mate_la_LIBADD =         \
	$(libeggsmclient_la_LIBADD)

libeggsmclient_mate_la_SOURCES =        \
	eggsmclient.c                    \
	eggsmclient.h                    \
	eggsmclient-private.h            \
	$(platform_sources)

libeggdesktopfile_la_LIBADD =            \
	$(EGG_SMCLIENT_LIBS)

libeggdesktopfile_la_SOURCES =           \
	eggdesktopfile.c                 \
	eggdesktopfile.h

-include $(top_srcdir)/git.mk
