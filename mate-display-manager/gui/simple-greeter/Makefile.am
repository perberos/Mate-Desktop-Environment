NULL =

SUBDIRS = 				\
	libnotificationarea		\
	$(NULL)

AM_CPPFLAGS = \
	-I$(top_srcdir)/common				\
	-I$(top_builddir)/common			\
	-I$(top_srcdir)/gui/simple-greeter/libnotificationarea	\
	-DDMCONFDIR=\""$(dmconfdir)"\"			\
	-DGDMCONFDIR=\"$(gdmconfdir)\"                  \
	-DDATADIR=\""$(datadir)"\"		 	\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBLOCALEDIR=\""$(prefix)/lib/locale"\"	\
	-DMATELOCALEDIR=\""$(datadir)/locale"\" 	\
	-DUIDIR=\""$(pkgdatadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\" 		\
	-DSBINDIR=\""$(sbindir)"\"		 	\
	-DGDM_CACHE_DIR=\""$(localstatedir)/cache/gdm"\"	\
	-DAT_SPI_REGISTRYD_DIR="\"$(AT_SPI_REGISTRYD_DIR)\""	\
	$(UPOWER_CFLAGS)				\
	$(DISABLE_DEPRECATED_CFLAGS)			\
	$(GTK_CFLAGS)					\
	$(SIMPLE_GREETER_CFLAGS)			\
	$(NULL)

@INTLTOOL_SCHEMAS_RULE@

schemasdir   = @MATECONF_SCHEMA_FILE_DIR@
schemas_in_files = gdm-simple-greeter.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

noinst_LTLIBRARIES = 			\
	libgdmuser.la			\
	$(null)

libgdmuser_la_SOURCES =			\
	gdm-user.c			\
	gdm-user.h			\
	gdm-user-private.h		\
	gdm-user-manager.c		\
	gdm-user-manager.h		\
	$(NULL)

libgdmuser_la_CFLAGS =			\
	$(SIMPLE_GREETER_CFLAGS)	\
	$(NULL)

libgdmuser_la_LIBADD =			\
	$(NULL)

libgdmuser_la_LDFLAGS = 	\
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
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-greeter-login-window.h	\
	gdm-greeter-login-window.c	\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-sessions.h			\
	gdm-sessions.c			\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-user-chooser-widget.h	\
	gdm-user-chooser-widget.c	\
	gdm-user-chooser-dialog.h	\
	gdm-user-chooser-dialog.c	\
	$(NULL)

test_greeter_login_window_LDADD =	\
	$(top_builddir)/common/libgdmcommon.la	\
	libgdmuser.la			\
	$(COMMON_LIBS)			\
	$(SIMPLE_GREETER_LIBS)		\
	$(RBAC_LIBS)			\
	$(NULL)

test_greeter_panel_SOURCES = 	\
	test-greeter-panel.c	\
	gdm-greeter-panel.h	\
	gdm-greeter-panel.c	\
	gdm-clock-widget.h      \
	gdm-clock-widget.c      \
	gdm-option-widget.h     \
	gdm-option-widget.c     \
	gdm-recent-option-widget.h	\
	gdm-recent-option-widget.c	\
	gdm-languages.h			\
	gdm-languages.c			\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-language-chooser-widget.h	\
	gdm-language-chooser-widget.c	\
	locarchive.h			\
	gdm-language-chooser-dialog.h	\
	gdm-language-chooser-dialog.c	\
	gdm-language-option-widget.h	\
	gdm-language-option-widget.c	\
	gdm-layout-chooser-widget.h	\
	gdm-layout-chooser-widget.c	\
	gdm-layout-chooser-dialog.h	\
	gdm-layout-chooser-dialog.c	\
	gdm-layout-option-widget.h	\
	gdm-layout-option-widget.c	\
	gdm-layouts.h			\
	gdm-layouts.c			\
	gdm-sessions.h			\
	gdm-sessions.c			\
	gdm-session-option-widget.h	\
	gdm-session-option-widget.c	\
	$(NULL)

test_greeter_panel_LDADD =	\
	$(top_builddir)/common/libgdmcommon.la	\
	$(top_builddir)/gui/simple-greeter/libnotificationarea/libnotificationarea.la	\
	$(SIMPLE_GREETER_LIBS)		\
	$(GTK_LIBS)			\
	$(MATECONF_LIBS)			\
	$(LIBXKLAVIER_LIBS)		\
	$(UPOWER_LIBS)		\
	$(NULL)

test_remote_login_window_SOURCES = 	\
	test-remote-login-window.c	\
	gdm-remote-login-window.h	\
	gdm-remote-login-window.c	\
	$(NULL)

test_remote_login_window_LDADD =	\
	$(GTK_LIBS)			\
	$(top_builddir)/common/libgdmcommon.la	\
	$(NULL)

test_filesystem_type_SOURCES = 	\
	test-filesystem-type.c	\
	$(NULL)

test_filesystem_type_LDADD =	\
	$(COMMON_LIBS)			\
	$(NULL)

test_language_chooser_SOURCES = 	\
	test-language-chooser.c		\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-languages.h			\
	gdm-languages.c			\
	gdm-language-chooser-widget.h	\
	gdm-language-chooser-widget.c	\
	locarchive.h			\
	gdm-language-chooser-dialog.h	\
	gdm-language-chooser-dialog.c	\
	$(NULL)

test_language_chooser_LDADD =	\
	$(GTK_LIBS)		\
	$(MATECONF_LIBS)		\
	$(NULL)

test_layout_chooser_SOURCES = 	\
	test-layout-chooser.c		\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-layouts.h			\
	gdm-layouts.c			\
	gdm-layout-chooser-widget.h	\
	gdm-layout-chooser-widget.c	\
	gdm-layout-chooser-dialog.h	\
	gdm-layout-chooser-dialog.c	\
	$(NULL)

test_layout_chooser_LDADD =		\
	$(GTK_LIBS)			\
	$(MATECONF_LIBS)			\
	$(LIBXKLAVIER_LIBS)		\
	$(NULL)

test_languages_SOURCES = 		\
	test-languages.c		\
	gdm-languages.h			\
	gdm-languages.c			\
	locarchive.h			\
	$(NULL)

test_languages_LDADD =		\
	$(GTK_LIBS)		\
	$(NULL)

test_sessions_SOURCES = 		\
	test-sessions.c		\
	gdm-sessions.h			\
	gdm-sessions.c			\
	$(NULL)

test_sessions_LDADD =		\
	$(GTK_LIBS)		\
	$(NULL)

test_user_chooser_SOURCES = 		\
	test-user-chooser.c		\
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-user-chooser-widget.h	\
	gdm-user-chooser-widget.c	\
	gdm-user-chooser-dialog.h	\
	gdm-user-chooser-dialog.c	\
	$(NULL)

test_user_chooser_LDADD =	\
	libgdmuser.la		\
	$(top_builddir)/common/libgdmcommon.la	\
	$(COMMON_LIBS)		\
	$(SIMPLE_GREETER_LIBS)	\
	$(NULL)

test_user_manager_SOURCES = 	\
	test-user-manager.c	\
	$(NULL)

test_user_manager_LDADD =	\
	libgdmuser.la		\
	$(top_builddir)/common/libgdmcommon.la	\
	$(COMMON_LIBS)		\
	$(SIMPLE_GREETER_LIBS)	\
	$(NULL)

libexec_PROGRAMS =			\
	gdm-simple-greeter

gdm_simple_greeter_SOURCES =  		\
	greeter-main.c 			\
	gdm-timer.h			\
	gdm-timer.c			\
	gdm-cell-renderer-timer.h	\
	gdm-cell-renderer-timer.c	\
	gdm-scrollable-widget.h		\
	gdm-scrollable-widget.c		\
	gdm-chooser-widget.h		\
	gdm-chooser-widget.c		\
	gdm-greeter-client.h		\
	gdm-greeter-client.c		\
	gdm-greeter-session.h		\
	gdm-greeter-session.c		\
	gdm-greeter-login-window.c	\
	gdm-greeter-login-window.h	\
	gdm-remote-login-window.c	\
	gdm-remote-login-window.h	\
	gdm-greeter-panel.h		\
	gdm-greeter-panel.c		\
	gdm-clock-widget.h		\
	gdm-clock-widget.c		\
	gdm-option-widget.h		\
	gdm-option-widget.c		\
	gdm-recent-option-widget.h	\
	gdm-recent-option-widget.c	\
	gdm-languages.h			\
	gdm-languages.c			\
	gdm-language-chooser-widget.h	\
	gdm-language-chooser-widget.c	\
	gdm-layout-chooser-widget.h	\
	gdm-layout-chooser-widget.c	\
	gdm-layout-chooser-dialog.h	\
	gdm-layout-chooser-dialog.c	\
	gdm-layout-option-widget.h	\
	gdm-layout-option-widget.c	\
	gdm-layouts.h			\
	gdm-layouts.c			\
	locarchive.h			\
	gdm-language-chooser-dialog.h	\
	gdm-language-chooser-dialog.c	\
	gdm-language-option-widget.h	\
	gdm-language-option-widget.c	\
	gdm-sessions.h			\
	gdm-sessions.c			\
	gdm-session-option-widget.h	\
	gdm-session-option-widget.c	\
	gdm-user-chooser-widget.h	\
	gdm-user-chooser-widget.c	\
	$(NULL)

gdm_simple_greeter_LDADD = 		\
	$(top_builddir)/common/libgdmcommon.la	\
	libgdmuser.la			\
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
	gdm-greeter-login-window.ui		\
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
