BUILDDIR = $(top_builddir)/docs

REF_VERSION = $(VERSION)
FULL_VERSION = $(VERSION)

HTML_STYLE = $(PYGOBJECT_DATADIR)/pygobject/xsl/ref-html-style.xsl
PDF_STYLE = $(PYGOBJECT_DATADIR)/pygobject/xsl/pdf-style.xsl

XMLFILES = 							\
	reference/caja-python-ref.xml			\
	reference/caja-python-overview.xml			\
	reference/caja-python-overview-example.xml			\
	reference/caja-python-overview-methods.xml			\
	reference/caja-python-class-reference.xml			\
	reference/caja-python-provider-reference.xml			\
	reference/caja-python-column.xml   \
	reference/caja-python-column-provider.xml   \
	reference/caja-python-menu.xml  \
	reference/caja-python-menu-item.xml \
	reference/caja-python-menu-provider.xml \
	reference/caja-python-property-page.xml \
	reference/caja-python-property-page-provider.xml \
	reference/caja-python-location-widget-provider.xml \
	reference/caja-python-file-info.xml \
	reference/caja-python-info-provider.xml \
	reference/caja-python-enum-reference.xml \
	reference/caja-python-operation-result.xml

HTMLdir = $(datadir)/gtk-doc/html/caja-python
HTMLFILES =						\
	html/index.html					\
	html/index.sgml					\
	html/caja-python-overview.html \
	html/caja-python-overview-example.html \
	html/caja-python-overview-methods.html \
	html/caja-python-class-reference.html \
	html/caja-python-provider-reference.html \
	html/class-caja-python-column.html \
	html/class-caja-python-column-provider.html \
	html/class-caja-python-menu.html  \
	html/class-caja-python-menu-item.html  \
	html/class-caja-python-menu-provider.html  \
	html/class-caja-python-property-page.html  \
	html/class-caja-python-property-page-provider.html  \
	html/class-caja-python-location-widget-provider.html  \
	html/class-caja-python-file-info.html  \
	html/class-caja-python-info-provider.html  \
	html/caja-python-enum-reference.html \
	html/enum-caja-python-operation-result.html  \
	html/caja-python.devhelp

CSS_FILES = $(PYGOBJECT_PYGDOCS)/style.css
CSSdir = $(HTMLdir)
CSS_DATA = $(CSS_FILES)

BUILT_SOURCES = 		\
	reference/builddate.xml	\
	$(HTMLFILES)

CLEANFILES = 			\
	caja-python-ref.*	\
	reference/builddate.xml	\
	$(HTMLFILES)

EXTRA_DIST = 		\
	$(XMLFILES)

REFERENCE_DEPS = 	\
	reference 	\
	$(XMLFILES)

if ENABLE_GTK_DOC

reference/builddate.xml: $(REFERENCE_DEPS)
	$(PYTHON) -c 'import datetime; print datetime.date.today()' > $@

$(HTMLFILES): $(REFERENCE_DEPS)
	xsltproc --nonet --xinclude -o $(BUILDDIR)/html/ \
		 --path $(BUILDDIR)/reference:$(srcdir)/reference \
		 --stringparam gtkdoc.bookname "caja-python" \
		 --stringparam gtkdoc.version ${REF_VERSION} \
		 $(HTML_STYLE) $(srcdir)/reference/caja-python-ref.xml
	$(PYGOBJECT_FIXXREF) -i $(PYGOBJECT_PYGDOCS) $(BUILDDIR)/html 
	touch $@

endif
