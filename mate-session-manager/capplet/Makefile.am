bin_PROGRAMS = mate-session-properties

AM_CPPFLAGS =						\
	$(SESSION_PROPERTIES_CFLAGS)			\
	$(MATECONF_CFLAGS)					\
	-I$(top_srcdir)/egg				\
	-I$(top_srcdir)/mate-session			\
	-DLOCALE_DIR=\""$(datadir)/locale"\"		\
	-DGTKBUILDER_DIR=\""$(pkgdatadir)"\"		\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

mate_session_properties_SOURCES =			\
	main.c						\
	gsm-properties-dialog.h				\
	gsm-properties-dialog.c				\
	gsm-app-dialog.h				\
	gsm-app-dialog.c				\
	gsp-app.h					\
	gsp-app.c					\
	gsp-app-manager.h				\
	gsp-app-manager.c				\
	gsp-keyfile.h					\
	gsp-keyfile.c

mate_session_properties_LDADD =			\
	$(SESSION_PROPERTIES_LIBS)			\
	$(top_builddir)/egg/libeggdesktopfile.la 	\
	$(top_builddir)/mate-session/libgsmutil.la 	\
	$(MATECONF_LIBS)

-include $(top_srcdir)/git.mk
