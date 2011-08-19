INCLUDES = -DGTK_DISABLE_DEPRECATED \
           -DGDK_DISABLE_DEPRECATED \
           -DG_DISABLE_DEPRECATED

noinst_LTLIBRARIES = libeggsmclient.la

libeggsmclient_la_LIBADD = $(LIBEGGSMCLIENT_LIBS)           \
                           -lSM -lICE
libeggsmclient_la_CFLAGS = $(LIBEGGSMCLIENT_CFLAGS)         \
                           -DEGG_SM_CLIENT_BACKEND_XSMP     \
                           -DG_LOG_DOMAIN=\""EggSMClient"\" \
                           $(WARN_CFLAGS)
libeggsmclient_la_SOURCES = eggdesktopfile.h \
                            eggdesktopfile.c \
                            eggsmclient.h \
                            eggsmclient.c \
                            eggsmclient-private.h \
                            eggsmclient-xsmp.c
