bin_SCRIPTS   = $(MATECOMPONENT_SLAY)
EXTRA_SCRIPTS = matecomponent-slay.in 

sbin_PROGRAMS  = matecomponent-activation-sysconf

matecomponent_activation_sysconf_SOURCES =	\
	matecomponent-activation-sysconf.c

INCLUDES =						\
	$(SERVER_CFLAGS)				\
	$(WARN_CFLAGS)					\
	-I$(top_srcdir)					\
	-DSERVER_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DSERVER_CONFDIR=\"$(sysconfdir)\"

matecomponent_activation_sysconf_LDADD =	\
	$(SERVER_LIBS)		\
	$(top_builddir)/matecomponent-activation/libmatecomponent-activation.la \
	$(INTLLIBS)

