include $(top_srcdir)/mate-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = system-admin-guide
DOC_ENTITIES =			\
	appendixa.xml		\
	appendixb.xml		\
	fontconfig.xml		\
	mateconf.xml		\
	glossary.xml		\
	help.xml		\
	lockdown.xml		\
	menustructure.xml	\
	mimetypes.xml		\
	performance.xml		\
	screensavers.xml	\
	sessions.xml		\
	themes.xml		\
	legal.xml

DOC_LINGUAS = ca cs de el es eu fi fr gl it ja ko oc pa pt sv zh_CN

install-data-local: install-doc-pdf
install-doc-pdf:
	for lc in C $(DOC_LINGUAS); do \
	  pdf="$$lc/$(DOC_MODULE).pdf"; \
	  if [ -f "$$pdf" ]; then spdf="$$pdf"; else spdf="$(srcdir)/$$pdf"; fi; \
	  if [ -f "$$spdf" ]; then \
	    $(mkinstalldirs) "$(DESTDIR)$(HELP_DIR)/$(DOC_MODULE)/$$lc"; \
	    $(INSTALL_DATA) "$$spdf" "$(DESTDIR)$(HELP_DIR)/$(DOC_MODULE)/$$pdf"; \
	  fi; \
	done
