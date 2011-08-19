include $(top_srcdir)/mate-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = mate-access-guide
DOC_ENTITIES =			\
	accessintro.xml		\
	preface.xml		\
	sysadmin.xml		\
	enable.xml  \
	general.xml \
	low-vision.xml  \
	mobility.xml	\
	legal.xml

DOC_LINGUAS = de el en_GB es eu fi fr gl hu it oc pt ru sv uk zh_CN

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

