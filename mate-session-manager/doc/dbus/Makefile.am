SPEC_XML_FILES =					\
	mate-session.xml				\
	org.mate.SessionManager.ref.xml		\
	org.mate.SessionManager.Client.ref.xml		\
	org.mate.SessionManager.ClientPrivate.ref.xml	\
	org.mate.SessionManager.Inhibitor.ref.xml	\
	org.mate.SessionManager.Presence.ref.xml

if DOCBOOK_DOCS_ENABLED

htmldocdir = $(docdir)/dbus
htmldoc_DATA = mate-session.html

mate-session.html: $(SPEC_XML_FILES)
	$(AM_V_GEN)$(XMLTO) xhtml-nochunks -m $(top_srcdir)/doc/dbus/config.xsl mate-session.xml

endif # DOCBOOK_DOCS_ENABLED

org.mate.SessionManager.ref.xml: $(top_srcdir)/mate-session/org.mate.SessionManager.xml spec-to-docbook.xsl
	$(AM_V_GEN)$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 > $@
org.mate.SessionManager.Client.ref.xml: $(top_srcdir)/mate-session/org.mate.SessionManager.Client.xml spec-to-docbook.xsl
	$(AM_V_GEN)$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 > $@
org.mate.SessionManager.ClientPrivate.ref.xml: $(top_srcdir)/mate-session/org.mate.SessionManager.ClientPrivate.xml spec-to-docbook.xsl
	$(AM_V_GEN)$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 > $@
org.mate.SessionManager.Inhibitor.ref.xml: $(top_srcdir)/mate-session/org.mate.SessionManager.Inhibitor.xml spec-to-docbook.xsl
	$(AM_V_GEN)$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 > $@
org.mate.SessionManager.Presence.ref.xml: $(top_srcdir)/mate-session/org.mate.SessionManager.Presence.xml spec-to-docbook.xsl
	$(AM_V_GEN)$(XSLTPROC) $(top_srcdir)/doc/dbus/spec-to-docbook.xsl $< | tail -n +2 > $@

BUILT_SOURCES =	\
	org.mate.SessionManager.ref.xml \
	org.mate.SessionManager.Client.ref.xml \
	org.mate.SessionManager.ClientPrivate.ref.xml \
	org.mate.SessionManager.Inhibitor.ref.xml \
	org.mate.SessionManager.Presence.ref.xml

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(htmldoc_DATA)

EXTRA_DIST =				\
	mate-session.xml.in		\
	config.xsl			\
	docbook.css			\
	dbus-introspect-docs.dtd	\
	spec-to-docbook.xsl

-include $(top_srcdir)/git.mk
