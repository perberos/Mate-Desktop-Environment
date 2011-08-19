# Not using INTLTOOL_XML_RULE because of it causes db-format.xml to be
# rebuilt in the tarball, which causes docs to be rebuilt, which causes
# translated docs to be rebuilt, which requires xml2po, which breaks
# --disable-build-utils

xsldir=$(datadir)/xml/mate/xslt/docbook/common

xsl_DATA =			\
	db-chunk.xsl		\
	db-common.xsl		\
	db-label.xsl		\
	db-title.xsl		\
	db-xref.xsl

EXTRA_DIST=$(xsl_DATA)
