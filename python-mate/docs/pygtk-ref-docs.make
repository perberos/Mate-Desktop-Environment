# -*- makefile -*-

HTML_DIR = $(datadir)/gtk-doc/html
TARGET_DIR = $(HTML_DIR)/$(REFERENCE_DOC_NAME)

EXTRA_DIST = $(REFERENCE_XML_FILES) $(REFERENCE_MAIN_FILE)

XSL_FILES =					\
	$(top_srcdir)/docs/common.xsl		\
	$(top_srcdir)/docs/html.xsl		\
	$(top_srcdir)/docs/ref-html-style.xsl	\
	$(top_srcdir)/docs/tut-html-style.xsl	\
	$(top_srcdir)/docs/pdf-style.xsl	\
	$(top_srcdir)/docs/pdf.xsl		\
	$(top_srcdir)/docs/devhelp.xsl

REFERENCE_FO = $(REFERENCE_MAIN_FILE:.xml=.o)
REFERENCE_PDF = $(REFERENCE_MAIN_FILE:.xml=.pdf)
REFERENCE_DEVHELP = $(REFERENCE_MAIN_FILE:.xml=.devhelp)

html.stamp: ${XSL_FILES} $(REFERENCE_XML_FILES) $(REFERENCE_MAIN_FILE)
	@echo '*** Building HTML ***'
	@-chmod -R u+w $(srcdir)
	rm -rf $(srcdir)/html/* 
	@-mkdir $(srcdir)/html
	xsltproc --nonet --xinclude -o $(srcdir)/html/				\
                 --stringparam gtkdoc.bookname $(REFERENCE_DOC_NAME)		\
                 --stringparam gtkdoc.version $(VERSION)			\
		 --stringparam chunker.output.encoding UTF-8			\
		$(top_srcdir)/docs/ref-html-style.xsl $(REFERENCE_MAIN_FILE)
	@echo '-- Fixing Crossreferences' 
	cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	touch $@

$(REFERENCE_PDF): $(XSL_FILES) $(REFERENCE_XML_FILES) $(REFERENCE_MAIN_FILE)
	xsltproc --nonet --xinclude -o $(REFERENCE_FO)	\
		$(top_srcdir)/docs/pdf-style.xsl	\
		$(REFERENCE_MAIN_FILE)
	pdfxmltex $(REFERENCE_FO) > output < /dev/null
	pdfxmltex $(REFERENCE_FO) > output < /dev/null
	pdfxmltex $(REFERENCE_FO) > output < /dev/null

$(DOC_MODULE).tar.bz2: html.stamp install
	builddir=`pwd` && cd $(HTML_DIR) && tar jcf $$builddir/$@ $(DOC_MODULE)

tarball: $(DOC_MODULE).tar.bz2

CLEANFILES = $(REFERENCE_FO) $(REFERENCE_PDF) *.aux *.log *.out output

maintainer-clean:
	-rm -rf html/* html.stamp


install-data-local:
	installfiles=`echo $(srcdir)/html/*`;			\
	if test "$$installfiles" = '$(srcdir)/html/*';		\
	then echo '-- Nothing to install' ;			\
	else							\
	  $(mkinstalldirs) $(DESTDIR)$(TARGET_DIR);		\
	  for i in $$installfiles; do				\
	    echo '-- Installing '$$i ;				\
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR);	\
	  done;							\
	fi

uninstall-local:
	rm -f $(DESTDIR)$(TARGET_DIR)/*


dist-hook: dist-hook-local html.stamp
	mkdir $(distdir)/html
	cp $(srcdir)/html/* $(distdir)/html
	cp $(srcdir)/html.stamp $(distdir)

.PHONY : dist-hook-local
