NULL =

SUBDIRS = reference

SPEC_XML_FILES =		\
	notification-spec.xml	\
	$(NULL)

if DOCBOOK_DOCS_ENABLED

htmldocdir = $(docdir)/spec
htmldoc_DATA = notification-spec.html

notification-spec.html: $(SPEC_XML_FILES)
	$(XMLTO) $(XMLTO_FLAGS) xhtml-nochunks -m $(top_srcdir)/docs/config.xsl notification-spec.xml

endif # DOCBOOK_DOCS_ENABLED


EXTRA_DIST =			\
	ChangeLog		\
	$(SPEC_XML_FILES)	\
	$(NULL)

-include $(top_srcdir)/git.mk
