NULL =

SUBDIRS =			\
	faces 			\
	greeter-autostart	\
	pixmaps			\
	$(NULL)

initdir = $(mdmconfdir)/Init
authdir = $(localstatedir)/mdm
postdir = $(mdmconfdir)/PostSession
predir = $(mdmconfdir)/PreSession
postlogindir = $(mdmconfdir)/PostLogin
workingdir = $(MDM_WORKING_DIR)
xauthdir = $(MDM_XAUTH_DIR)
screenshotdir = $(MDM_SCREENSHOT_DIR)
cachedir = $(localstatedir)/cache/mdm

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

mdm.conf-custom: $(srcdir)/mdm.conf-custom.in
	sed	-e 's,[@]MDM_DEFAULTS_CONF[@],$(MDM_DEFAULTS_CONF),g' \
		<$(srcdir)/mdm.conf-custom.in >mdm.conf-custom

dbusconfdir = $(DBUS_SYS_DIR)
dbusconf_in_files = mdm.conf.in
dbusconf_DATA = $(dbusconf_in_files:.conf.in=.conf)

@INTLTOOL_SCHEMAS_RULE@

schemasdir = $(pkgdatadir)
schemas_in_files = mdm.schemas.in
schemas_DATA = $(schemas_in_files:.schemas.in=.schemas)

mdm.schemas.in: $(srcdir)/mdm.schemas.in.in
	sed	-e 's,[@]MDMPREFETCHCMD[@],$(MDMPREFETCHCMD),g' \
		-e 's,[@]MDM_CUSTOM_CONF[@],$(MDM_CUSTOM_CONF),g' \
		-e 's,[@]MDM_USER_PATH[@],$(MDM_USER_PATH),g' \
		-e 's,[@]MDM_USERNAME[@],$(MDM_USERNAME),g' \
		-e 's,[@]MDM_GROUPNAME[@],$(MDM_GROUPNAME),g' \
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
		-e 's,[@]MDM_RBAC_SYSCMD_KEYS[@],$(MDM_RBAC_SYSCMD_KEYS),g' \
		-e 's,[@]authdir[@],$(authdir),g' \
		-e 's,[@]datadir[@],$(datadir),g' \
		-e 's,[@]mdmconfdir[@],$(mdmconfdir),g' \
		-e 's,[@]libdir[@],$(libdir),g' \
		-e 's,[@]libexecdir[@],$(libexecdir),g' \
		-e 's,[@]mdmlocaledir[@],$(mdmlocaledir),g' \
		-e 's,[@]logdir[@],$(logdir),g' \
		-e 's,[@]pixmapdir[@],$(pixmapdir),g' \
		-e 's,[@]sbindir[@],$(sbindir),g' \
		<$(srcdir)/mdm.schemas.in.in >mdm.schemas.in

localealiasdir = $(datadir)/mdm
localealias_DATA = locale.alias

EXTRA_DIST =			\
	$(schemas_in_files)	\
	$(schemas_DATA)		\
	$(dbusconf_in_files)	\
	$(localealias_DATA)	\
	mdm.schemas.in.in	\
	mdm.conf-custom.in 	\
	Xsession.in 		\
	mdm 			\
	mdm-autologin 		\
	Init.in 		\
	PreSession.in 		\
	PostSession.in 		\
	PostLogin 		\
	mateconf.path		\
	session-setup.entries	\
	$(NULL)

CLEANFILES = 				\
	Xsession			\
	mdm.conf-custom			\
	Init				\
	PreSession			\
	PostSession			\
	$(NULL)

DISTCLEANFILES =			\
	$(dbusconf_DATA)			\
	mdm.schemas			\
	$(NULL)

MAINTAINERCLEANFILES =			\
	*~				\
	Makefile.in


uninstall-hook:
	rm -f \
	$(DESTDIR)$(MDM_CUSTOM_CONF) \
	$(DESTDIR)$(mdmconfdir)/Xsession \
	$(DESTDIR)$(initdir)/Default \
	$(DESTDIR)$(postlogindir)/Default.sample \
	$(DESTDIR)$(predir)/Default \
	$(DESTDIR)$(postdir)/Default \
	$(DESTDIR)$(PAM_PREFIX)/pam.d/mdm \
	$(DESTDIR)$(PAM_PREFIX)/pam.d/mdm-autologin \
	$(DESTDIR)$(workingdir)/.mateconf.path \
	-rf \
	$(DESTDIR)$(workingdir)/.mateconf.mandatory \
	$(DESTDIR)$(screenshotdir) \
	$(DESTDIR)$(xauthdir)

install-data-hook: mdm.conf-custom Xsession Init PostSession PreSession mateconf.path
	if test '!' -d $(DESTDIR)$(mdmconfdir); then \
		$(mkinstalldirs) $(DESTDIR)$(mdmconfdir); \
		chmod 755 $(DESTDIR)$(mdmconfdir); \
	fi
	if test '!' -f $(DESTDIR)$(MDM_CUSTOM_CONF); then \
		$(INSTALL_DATA) mdm.conf-custom $(DESTDIR)$(MDM_CUSTOM_CONF); \
		chmod 644 $(DESTDIR)$(MDM_CUSTOM_CONF); \
	fi

	$(INSTALL_SCRIPT) Xsession $(DESTDIR)$(mdmconfdir)/Xsession

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
		chown root:mdm $(DESTDIR)$(authdir) || : ; \
	fi

	system=`uname`; \
	if test -f /usr/include/security/pam_appl.h; then \
	  if test '!' -d $(DESTDIR)$(PAM_PREFIX)/pam.d; then \
		$(mkinstalldirs) $(DESTDIR)$(PAM_PREFIX)/pam.d; \
		chmod 755 $(DESTDIR)$(PAM_PREFIX)/pam.d; \
	   fi; \
	   if test $$system = Linux && test '!' -f $(DESTDIR)$(PAM_PREFIX)/pam.d/mdm; then \
		$(INSTALL_DATA) $(srcdir)/mdm $(DESTDIR)$(PAM_PREFIX)/pam.d/mdm; \
	   fi; \
	   if test $$system = Linux && test '!' -f $(DESTDIR)$(PAM_PREFIX)/pam.d/mdm-autologin; then \
		$(INSTALL_DATA) $(srcdir)/mdm-autologin $(DESTDIR)$(PAM_PREFIX)/pam.d/mdm-autologin; \
	   fi; \
	   if test $$system = SunOS; then \
		echo "Please add PAM authentication for mdm and mdm-autologin in $(PAM_PREFIX)/pam.conf!"; \
	   fi; \
	fi

	if test '!' -d $(DESTDIR)$(xauthdir); then \
		$(mkinstalldirs) $(DESTDIR)$(xauthdir); \
		chmod 0711 $(DESTDIR)$(xauthdir); \
		chown root:mdm $(DESTDIR)$(xauthdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(screenshotdir); then \
		$(mkinstalldirs) $(DESTDIR)$(screenshotdir); \
		chmod 0755 $(DESTDIR)$(screenshotdir); \
		chown mdm:mdm $(DESTDIR)$(screenshotdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(workingdir); then \
		$(mkinstalldirs) $(DESTDIR)$(workingdir); \
		chmod 1770 $(DESTDIR)$(workingdir); \
		chown root:mdm $(DESTDIR)$(workingdir) || : ; \
	fi

	if test '!' -d $(DESTDIR)$(cachedir); then \
		$(mkinstalldirs) $(DESTDIR)$(cachedir); \
		chmod 1755 $(DESTDIR)$(cachedir); \
		chown root:mdm $(DESTDIR)$(cachedir) || : ; \
	fi

	$(INSTALL_DATA) $(srcdir)/mateconf.path $(DESTDIR)$(workingdir)/.mateconf.path
	mateconftool-2 --direct --config-source=xml:merged:$(DESTDIR)$(workingdir)/.mateconf.mandatory --recursive-unset /
	mateconftool-2 --direct --config-source=xml:merged:$(DESTDIR)$(workingdir)/.mateconf.mandatory --load $(srcdir)/session-setup.entries
	chown -R root:mdm $(DESTDIR)$(workingdir)/.mateconf.mandatory || :
	chmod 1750 $(DESTDIR)$(workingdir)/.mateconf.mandatory
	chmod 1640 $(DESTDIR)$(workingdir)/.mateconf.mandatory/*.xml


