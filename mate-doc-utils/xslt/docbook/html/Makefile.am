xsldir=$(datadir)/xml/mate/xslt/docbook/html

xsl_DATA =				\
	db2html-autotoc.xsl		\
	db2html-bibliography.xsl	\
	db2html-block.xsl		\
	db2html-callout.xsl		\
	db2html-classsynopsis.xsl	\
	db2html-cmdsynopsis.xsl		\
	db2html-css.xsl			\
	db2html-division.xsl		\
	db2html-ebnf.xsl		\
	db2html-footnote.xsl		\
	db2html-funcsynopsis.xsl	\
	db2html-index.xsl		\
	db2html-info.xsl		\
	db2html-inline.xsl		\
	db2html-l10n.xsl		\
	db2html-media.xsl		\
	db2html-list.xsl		\
	db2html-qanda.xsl		\
	db2html-refentry.xsl		\
	db2html-suppressed.xsl		\
	db2html-table.xsl		\
	db2html-title.xsl		\
	db2html-xref.xsl		\
	db2html.xsl			\
	db2xhtml.xsl

EXTRA_DIST=$(xsl_DATA)

report.xml: $(wildcard db*.xsl) elements report.sh report.xslt
	./report.sh | xsltproc -o report.xml report.xslt -
report.html: report.xml report.xslt
	xsltproc -o report.html report.xslt report.xml

