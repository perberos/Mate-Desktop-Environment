NULL =

SUBDIRS = 				\
	libnotificationarea		\
	$(NULL)

AM_CPPFLAGS = \
	-I$(top_srcdir)/common				\
	-I$(top_builddir)/common			\
	-I$(top_srcdir)/gui/simple-greeter/libnotificationarea	\
	-DDMCONFDIR=\""$(dmconfdir)"\"			\
	-DMDMCONFDIR=\"$(mdmconfdir)\"                  \
	-DDATADIR=\""$(datadir)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBLOCALEDIR=\""$(prefix)/lib/locale"\"	\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DUIDIR=\""$(pkgdatadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\" 		\
	-DSBINDIR=\""$(sbindir)"\"		 	\
	-DMDM_CACHE_DIR=\""$(localstatedir)/cache/mdm"\"	\
	-DAT_SPI_REGISTRYD_DIR="\"$(AT_SPI_REGISTRYD_DIR)\""	\
	$(UPOWER_CFLAGS)				\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(GTK_CFLAGS)					\
	$(SIMPLE_GREETER_CFLAGS)			\
	$(NULL)

@INTLTOOL_SCHEMAS_RULE@

schemasdir   = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = mdm-simple-greeter.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

noinst_LTLIBRARIES = 			\
	libmdmuser.la			\
	$(null)

libmdmuser_la_SOURCES =			\
	mdm-user.c			\
	mdm-user.h			\
	mdm-user-private.h		\
	mdm-user-manager.c		\
	mdm-user-manager.h		\
	$(NULL)

libmdmuser_la_CFLAGS =			\
	$(SIMPLE_GREETER_CFLAGS)	\
	$(NULL)

libmdmuser_la_LIBADD =			\
	$(NULL)

libmdmuser_la_LDFLAGS = 	\
	-export-dynamic 	\
	$(NULL)

noinst_PROGRAMS = 			\
	test-filesystem-type		\
	test-greeter-login-window	\
	test-greeter-panel		\
	test-language-chooser		\
	test-layout-chooser		\
	test-languages			\
	test-sessions			\
	test-remote-login-window	\
	test-user-chooser		\
	test-user-manager		\
	$(NULL)

test_greeter_login_window_SOURCES = 	\
	test-greeter-login-window.c	\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-greeter-login-window.h	\
	mdm-greeter-login-window.c	\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-sessions.h			\
	mdm-sessions.c			\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-user-chooser-widget.h	\
	mdm-user-chooser-widget.c	\
	mdm-user-chooser-dialog.h	\
	mdm-user-chooser-dialog.c	\
	$(NULL)

test_greeter_login_window_LDADD =	\
	$(top_builddir)/common/libmdmcommon.la	\
	libmdmuser.la			\
	$(COMMON_LIBS)			\
	$(SIMPLE_GREETER_LIBS)		\
	$(RBAC_LIBS)			\
	$(NULL)

test_greeter_panel_SOURCES = 	\
	test-greeter-panel.c	\
	mdm-greeter-panel.h	\
	mdm-greeter-panel.c	\
	mdm-clock-widget.h      \
	mdm-clock-widget.c      \
	mdm-option-widget.h     \
	mdm-option-widget.c     \
	mdm-recent-option-widget.h	\
	mdm-recent-option-widget.c	\
	mdm-languages.h			\
	mdm-languages.c			\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-language-chooser-widget.h	\
	mdm-language-chooser-widget.c	\
	locarchive.h			\
	mdm-language-chooser-dialog.h	\
	mdm-language-chooser-dialog.c	\
	mdm-language-option-widget.h	\
	mdm-language-option-widget.c	\
	mdm-layout-chooser-widget.h	\
	mdm-layout-chooser-widget.c	\
	mdm-layout-chooser-dialog.h	\
	mdm-layout-chooser-dialog.c	\
	mdm-layout-option-widget.h	\
	mdm-layout-option-widget.c	\
	mdm-layouts.h			\
	mdm-layouts.c			\
	mdm-sessions.h			\
	mdm-sessions.c			\
	mdm-session-option-widget.h	\
	mdm-session-option-widget.c	\
	$(NULL)

test_greeter_panel_LDADD =	\
	$(top_builddir)/common/libmdmcommon.la	\
	$(top_builddir)/gui/simple-greeter/libnotificationarea/libnotificationarea.la	\
	$(SIMPLE_GREETER_LIBS)		\
	$(GTK_LIBS)			\
	$(MATECONF_LIBS)			\
	$(LIBXKLAVIER_LIBS)		\
	$(UPOWER_LIBS)		\
	$(NULL)

test_remote_login_window_SOURCES = 	\
	test-remote-login-window.c	\
	mdm-remote-login-window.h	\
	mdm-remote-login-window.c	\
	$(NULL)

test_remote_login_window_LDADD =	\
	$(GTK_LIBS)			\
	$(top_builddir)/common/libmdmcommon.la	\
	$(NULL)

test_filesystem_type_SOURCES = 	\
	test-filesystem-type.c	\
	$(NULL)

test_filesystem_type_LDADD =	\
	$(COMMON_LIBS)			\
	$(NULL)

test_language_chooser_SOURCES = 	\
	test-language-chooser.c		\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-languages.h			\
	mdm-languages.c			\
	mdm-language-chooser-widget.h	\
	mdm-language-chooser-widget.c	\
	locarchive.h			\
	mdm-language-chooser-dialog.h	\
	mdm-language-chooser-dialog.c	\
	$(NULL)

test_language_chooser_LDADD =	\
	$(GTK_LIBS)		\
	$(MATECONF_LIBS)		\
	$(NULL)

test_layout_chooser_SOURCES = 	\
	test-layout-chooser.c		\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-layouts.h			\
	mdm-layouts.c			\
	mdm-layout-chooser-widget.h	\
	mdm-layout-chooser-widget.c	\
	mdm-layout-chooser-dialog.h	\
	mdm-layout-chooser-dialog.c	\
	$(NULL)

test_layout_chooser_LDADD =		\
	$(GTK_LIBS)			\
	$(MATECONF_LIBS)			\
	$(LIBXKLAVIER_LIBS)		\
	$(NULL)

test_languages_SOURCES = 		\
	test-languages.c		\
	mdm-languages.h			\
	mdm-languages.c			\
	locarchive.h			\
	$(NULL)

test_languages_LDADD =		\
	$(GTK_LIBS)		\
	$(NULL)

test_sessions_SOURCES = 		\
	test-sessions.c		\
	mdm-sessions.h			\
	mdm-sessions.c			\
	$(NULL)

test_sessions_LDADD =		\
	$(GTK_LIBS)		\
	$(NULL)

test_user_chooser_SOURCES = 		\
	test-user-chooser.c		\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-user-chooser-widget.h	\
	mdm-user-chooser-widget.c	\
	mdm-user-chooser-dialog.h	\
	mdm-user-chooser-dialog.c	\
	$(NULL)

test_user_chooser_LDADD =	\
	libmdmuser.la		\
	$(top_builddir)/common/libmdmcommon.la	\
	$(COMMON_LIBS)		\
	$(SIMPLE_GREETER_LIBS)	\
	$(NULL)

test_user_manager_SOURCES = 	\
	test-user-manager.c	\
	$(NULL)

test_user_manager_LDADD =	\
	libmdmuser.la		\
	$(top_builddir)/common/libmdmcommon.la	\
	$(COMMON_LIBS)		\
	$(SIMPLE_GREETER_LIBS)	\
	$(NULL)

libexec_PROGRAMS =			\
	mdm-simple-greeter

mdm_simple_greeter_SOURCES =  		\
	greeter-main.c 			\
	mdm-timer.h			\
	mdm-timer.c			\
	mdm-cell-renderer-timer.h	\
	mdm-cell-renderer-timer.c	\
	mdm-scrollable-widget.h		\
	mdm-scrollable-widget.c		\
	mdm-chooser-widget.h		\
	mdm-chooser-widget.c		\
	mdm-greeter-client.h		\
	mdm-greeter-client.c		\
	mdm-greeter-session.h		\
	mdm-greeter-session.c		\
	mdm-greeter-login-window.c	\
	mdm-greeter-login-window.h	\
	mdm-remote-login-window.c	\
	mdm-remote-login-window.h	\
	mdm-greeter-panel.h		\
	mdm-greeter-panel.c		\
	mdm-clock-widget.h		\
	mdm-clock-widget.c		\
	mdm-option-widget.h		\
	mdm-option-widget.c		\
	mdm-recent-option-widget.h	\
	mdm-recent-option-widget.c	\
	mdm-languages.h			\
	mdm-languages.c			\
	mdm-language-chooser-widget.h	\
	mdm-language-chooser-widget.c	\
	mdm-layout-chooser-widget.h	\
	mdm-layout-chooser-widget.c	\
	mdm-layout-chooser-dialog.h	\
	mdm-layout-chooser-dialog.c	\
	mdm-layout-option-widget.h	\
	mdm-layout-option-widget.c	\
	mdm-layouts.h			\
	mdm-layouts.c			\
	locarchive.h			\
	mdm-language-chooser-dialog.h	\
	mdm-language-chooser-dialog.c	\
	mdm-language-option-widget.h	\
	mdm-language-option-widget.c	\
	mdm-sessions.h			\
	mdm-sessions.c			\
	mdm-session-option-widget.h	\
	mdm-session-option-widget.c	\
	mdm-user-chooser-widget.h	\
	mdm-user-chooser-widget.c	\
	$(NULL)

mdm_simple_greeter_LDADD = 		\
	$(top_builddir)/common/libmdmcommon.la	\
	libmdmuser.la			\
	$(top_builddir)/gui/simple-greeter/libnotificationarea/libnotificationarea.la	\
	$(COMMON_LIBS)			\
	$(EXTRA_GREETER_LIBS)   	\
	$(SIMPLE_GREETER_LIBS)		\
	$(RBAC_LIBS)			\
	$(LIBXKLAVIER_LIBS)		\
	$(UPOWER_LIBS)		\
	$(NULL)

uidir = $(pkgdatadir)
ui_DATA = 					\
	mdm-greeter-login-window.ui		\
	$(NULL)

EXTRA_DIST = 			\
	$(schemas_in_files)	\
	$(ui_DATA)		\
	$(NULL)

DISTCLEANFILES =                        \
        $(schemas_DATA)                 \
        $(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

if MATECONF_SCHEMAS_INSTALL
# don't do this if we are building in eg. rpm
install-data-local: $(schemas_DATA)
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
			MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/gui/simple-greeter/$$p ; \
		done \
	fi
else
install-data-local:
endif
