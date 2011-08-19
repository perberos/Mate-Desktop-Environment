NULL =

SUBDIRS =			\
	faces 			\
	greeter-autostart	\
	pixmaps			\
	$(NULL)

initdir = $(gdmconfdir)/Init
authdir = $(localstatedir)/gdm
postdir = $(gdmconfdir)/PostSession
predir = $(gdmconfdir)/PreSession
postlogindir = $(gdmconfdir)/PostLogin
workingdir = $(GDM_WORKING_DIR)
xauthdir = $(GDM_XAUTH_DIR)
screenshotdir = $(GDM_SCREENSHOT_DIR)
cachedir = $(localstatedir)/cache/gdm

Xsession: $(srcdir)/Xsession.in
	sed	-e 's,[@]XSESSION_SHELL[@],$(XSESSION_SHELL),g' \
		-e 's,[@]libexecdir[@],$(libexecdir),g' \
		<$(srcdir)/Xsession.in >Xsession

Init: $(srcdir)/Init.in
	sed	-e 's,[@]X_PATH[@],$(X_PATH),g' \
		<$(srcdir)/Init.in >Init

PreSession: $(srcdir)/PreSession.in
	sed	-e 's,[@]X_PATH[@],$(X_PATH),g' \
		<$(srcdir)/PreSession.in >PreSession
PostSession: $(srcdir)/PostSession.in
	sed	-e 's,[@]X_PATH[@],$(X_PATH),g' \
		<$(srcdir)/PostSession.in >PostSession

gdm.conf-custom: $(srcdir)/gdm.conf-custom.in
	sed	-e 's,[@]GDM_DEFAULTS_CONF[@],$(GDM_DEFAULTS_CONF),g' \
		<$(srcdir)/gdm.conf-custom.in >gdm.conf-custom

dbusconfdir = $(DBUS_SYS_DIR)
dbusconf_in_files = gdm.conf.in
dbusconf_DATA = $(dbusconf_in_files:.conf.in=.conf)

@INTLTOOL_SCHEMAS_RULE@

schemasdir = $(pkgdatadir)
schemas_in_files = gdm.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

gdm.schemas.in: $(srcdir)/gdm.schemas.in.in
	sed	-e 's,[@]GDMPREFETCHCMD[@],$(GDMPREFETCHCMD),g' \
		-e 's,[@]GDM_CUSTOM_CONF[@],$(GDM_CUSTOM_CONF),g' \
		-e 's,[@]GDM_USER_PATH[@],$(GDM_USER_PATH),g' \
		-e 's,[@]GDM_USERNAME[@],$(GDM_USERNAME),g' \
		-e 's,[@]GDM_GROUPNAME[@],$(GDM_GROUPNAME),g' \
		-e 's,[@]HALT_COMMAND[@],$(HALT_COMMAND),g' \
		-e 's,[@]REBOOT_COMMAND[@],$(REBOOT_COMMAND),g' \
		-e 's,[@]SOUND_PROGRAM[@],$(SOUND_PROGRAM),g' \
		-e 's,[@]SUSPEND_COMMAND[@],$(SUSPEND_COMMAND),g' \
		-e 's,[@]XEVIE_OPTION[@],$(XEVIE_OPTION),g' \
		-e 's,[@]ALWAYS_RESTART_SERVER[@],$(ALWAYS_RESTART_SERVER),g' \
		-e 's,[@]X_CONFIG_OPTIONS[@],$(X_CONFIG_OPTIONS),g' \
		-e 's,[@]X_SERVER[@],$(X_SERVER),g' \
		-e 's,[@]X_XNEST_CMD[@],$(X_XNEST_CMD),g' \
		-e 's,[@]X_XNEST_CONFIG_OPTIONS[@],$(X_XNEST_CONFIG_OPTIONS),g' \
		-e 's,[@]X_XNEST_UNSCALED_FONTPATH[@],$(X_XNEST_UNSCALED_FONTPATH),g' \
		-e 's,[@]GDM_RBAC_SYSCMD_KEYS[@],$(GDM_RBAC_SYSCMD_KEYS),g' \
		-e 's,[@]authdir[@],$(authdir),g' \
		-e 's,[@]datadir[@],$(datadir),g' \
		-e 's,[@]gdmconfdir[@],$(gdmconfdir),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]libexecdir[@],$(libexecdir),g' \
		-e 's,[@]gdmlocaledir[@],$(gdmlocaledir),g' \
		-e 's,[@]logdir[@],$(logdir),g' \
		-e 's,[@]pixmapdir[@],$(pixmapdir),g' \
		-e 's,[@]sbindir[@],$(sbindir),g' \
		<$(srcdir)/gdm.schemas.in.in >gdm.schemas.in

localealiasdir = $(datadir)/gdm
localealias_DATA = locale.alias

EXTRA_DIST =			\
	$(schemas_in_files)	\
	$(schemas_DATA)		\
	$(dbusconf_in_files)	\
	$(localealias_DATA)	\
	gdm.schemas.in.in	\
	gdm.conf-custom.in 	\
	Xsession.in 		\
	gdm 			\
	gdm-autologin 		\
	Init.in 		\
	PreSession.in 		\
	PostSession.in 		\
	PostLogin 		\
	mateconf.path		\
	session-setup.entries	\
	$(NULL)

CLEANFILES = 				\
	Xsession			\
	gdm.conf-custom			\
	Init				\
	PreSession			\
	PostSession			\
	$(NULL)

DISTCLEANFILES =			\
	$(dbusconf_DATA)			\
	gdm.schemas			\
	$(NULL)

MAINTAINERCLEANFILES =			\
	*~				\
	Makefile.in


uninstall-hook:
	rm -f \
	$(DESTDIR)$(GDM_CUSTOM_CONF) \
	$(DESTDIR)$(gdmconfdir)/Xsession \
	$(DESTDIR)$(initdir)/Default \
	$(DESTDIR)$(postlogindir)/Default.sample \
	$(DESTDIR)$(predir)/Default \
	$(DESTDIR)$(postdir)/Default \
	$(DESTDIR)$(PAM_PREFIX)/pam.d/gdm \
	$(DESTDIR)$(PAM_PREFIX)/pam.d/gdm-autologin \
	$(DESTDIR)$(workingdir)/.mateconf.path \
	-rf \
	$(DESTDIR)$(workingdir)/.mateconf.mandatory \
	$(DESTDIR)$(screenshotdir) \
	$(DESTDIR)$(xauthdir)

install-data-hook: gdm.conf-custom Xsession Init PostSession PreSession mateconf.path
	if test '!' -d $(DESTDIR)$(gdmconfdir); then \
		$(mkinstalldirs) $(DESTDIR)$(gdmconfdir); \
		chmod 755 $(DESTDIR)$(gdmconfdir); \
	fi
	if test '!' -f $(DESTDIR)$(GDM_CUSTOM_CONF); then \
		$(INSTALL_DATA) gdm.conf-custom $(DESTDIR)$(GDM_CUSTOM_CONF); \
		chmod 644 $(DESTDIR)$(GDM_CUSTOM_CONF); \
	fi

	$(INSTALL_SCRIPT) Xsession $(DESTDIR)$(gdmconfdir)/Xsession

	if test '!' -d $(DESTDIR)$(initdir); then \
		$(mkinstalldirs) $(DESTDIR)$(initdir); \
		chmod 755 $(DESTDIR)$(initdir); \
	fi
	-if test -f $(DESTDIR)$(initdir)/Default; then \
		cp -f $(DESTDIR)$(initdir)/Default $(DESTDIR)$(initdir)/Default.orig; \
	fi
	$(INSTALL_SCRIPT) Init $(DESTDIR)$(initdir)/Default

	if test '!' -d $(DESTDIR)$(postlogindir); then \
		$(mkinstalldirs) $(DESTDIR)$(postlogindir); \
		chmod 755 $(DESTDIR)$(postlogindir); \
	fi
	$(INSTALL_SCRIPT) $(srcdir)/PostLogin $(DESTDIR)$(postlogindir)/Default.sample

	if test '!' -d $(DESTDIR)$(predir); then \
		$(mkinstalldirs) $(DESTDIR)$(predir); \
		chmod 755 $(DESTDIR)$(predir); \
	fi
	-if test -f $(DESTDIR)$(predir)/Default; then \
		cp -f $(DESTDIR)$(predir)/Default $(DESTDIR)$(predir)/Default.orig; \
	fi
	$(INSTALL_SCRIPT) PreSession $(DESTDIR)$(predir)/Default

	if test '!' -d $(DESTDIR)$(postdir); then \
		$(mkinstalldirs) $(DESTDIR)$(postdir); \
		chmod 755 $(DESTDIR)$(postdir); \
	fi
	-if test -f $(DESTDIR)$(postdir)/Default; then \
		cp -f $(DESTDIR)$(postdir)/Default $(DESTDIR)$(postdir)/Default.orig; \
	fi
	$(INSTALL_SCRIPT) PostSession $(DESTDIR)$(postdir)/Default

	if test '!' -d $(DESTDIR)$(logdir); then \
		$(mkinstalldirs) $(DESTDIR)$(logdir); \
		chmod 755 $(DESTDIR)$(logdir); \
		chown root:root $(DESTDIR)$(logdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(authdir); then \
		$(mkinstalldirs) $(DESTDIR)$(authdir); \
		chmod 1770 $(DESTDIR)$(authdir); \
		chown root:gdm $(DESTDIR)$(authdir) || : ; \
	fi

	system=`uname`; \
	if test -f /usr/include/security/pam_appl.h; then \
	  if test '!' -d $(DESTDIR)$(PAM_PREFIX)/pam.d; then \
		$(mkinstalldirs) $(DESTDIR)$(PAM_PREFIX)/pam.d; \
		chmod 755 $(DESTDIR)$(PAM_PREFIX)/pam.d; \
	   fi; \
	   if test $$system = Linux && test '!' -f $(DESTDIR)$(PAM_PREFIX)/pam.d/gdm; then \
		$(INSTALL_DATA) $(srcdir)/gdm $(DESTDIR)$(PAM_PREFIX)/pam.d/gdm; \
	   fi; \
	   if test $$system = Linux && test '!' -f $(DESTDIR)$(PAM_PREFIX)/pam.d/gdm-autologin; then \
		$(INSTALL_DATA) $(srcdir)/gdm-autologin $(DESTDIR)$(PAM_PREFIX)/pam.d/gdm-autologin; \
	   fi; \
	   if test $$system = SunOS; then \
		echo "Please add PAM authentication for gdm and gdm-autologin in $(PAM_PREFIX)/pam.conf!"; \
	   fi; \
	fi

	if test '!' -d $(DESTDIR)$(xauthdir); then \
		$(mkinstalldirs) $(DESTDIR)$(xauthdir); \
		chmod 0711 $(DESTDIR)$(xauthdir); \
		chown root:gdm $(DESTDIR)$(xauthdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(screenshotdir); then \
		$(mkinstalldirs) $(DESTDIR)$(screenshotdir); \
		chmod 0755 $(DESTDIR)$(screenshotdir); \
		chown gdm:gdm $(DESTDIR)$(screenshotdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(workingdir); then \
		$(mkinstalldirs) $(DESTDIR)$(workingdir); \
		chmod 1770 $(DESTDIR)$(workingdir); \
		chown root:gdm $(DESTDIR)$(workingdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(cachedir); then \
		$(mkinstalldirs) $(DESTDIR)$(cachedir); \
		chmod 1755 $(DESTDIR)$(cachedir); \
		chown root:gdm $(DESTDIR)$(cachedir) || : ; \
	fi

	$(INSTALL_DATA) $(srcdir)/mateconf.path $(DESTDIR)$(workingdir)/.mateconf.path
	mateconftool-2 --direct --config-source=xml:merged:$(DESTDIR)$(workingdir)/.mateconf.mandatory --recursive-unset /
	mateconftool-2 --direct --config-source=xml:merged:$(DESTDIR)$(workingdir)/.mateconf.mandatory --load $(srcdir)/session-setup.entries
	chown -R root:gdm $(DESTDIR)$(workingdir)/.mateconf.mandatory || :
	chmod 1750 $(DESTDIR)$(workingdir)/.mateconf.mandatory
	chmod 1640 $(DESTDIR)$(workingdir)/.mateconf.mandatory/*.xml


