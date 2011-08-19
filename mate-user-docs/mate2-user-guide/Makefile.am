include $(top_srcdir)/mate-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = user-guide
DOC_ENTITIES = legal.xml
DOC_INCLUDES =			\
	gosbasic.xml		\
	goscustdesk.xml		\
	goseditmainmenu.xml	\
	goscaja.xml		\
	gosoverview.xml		\
	gospanel.xml		\
	gosstartsession.xml	\
	gostools.xml		\
	gosfeedback.xml		\
	glossary.xml

DOC_LINGUAS = ar bg cs de el es eu fi fr gl hu id it ja ko oc pa pt pt_BR ru sv th uk zh_CN

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
