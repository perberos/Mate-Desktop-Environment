# Not using INTLTOOL_XML_RULE because of it causes l10n.xml to be rebuilt
# in the tarball, which causes docs to be rebuilt, which causes translated
# docs to be rebuilt, which requires xml2po, which breaks --disable-build-utils
l10n.xml: l10n.xml.in $(wildcard $(top_srcdir)/po/*.po)
	LC_ALL=C $(INTLTOOL_MERGE) -x -u -c $(top_builddir)/po/.intltool-merge-cache $(top_srcdir)/po $< $@
	xmllint --noout $@ || (mv $@ $@.broken && false)

xsldir=$(datadir)/xml/mate/xslt/gettext

xsl_DATA =			\
	gettext.xsl		\
	l10n-numbers.xsl	\
	l10n.xml

EXTRA_DIST=$(xsl_DATA) l10n.xml.in

CLEANFILES = l10n.xml
