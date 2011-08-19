include $(top_srcdir)/tools/mate-doc-utils.make
include $(top_srcdir)/bootstrap.make
dist-hook: doc-dist-hook

DOC_MODULE = mate-doc-make
DOC_INCLUDES = legal.xml make-ref.xml

$(srcdir)/C/make-ref.xml: $(top_srcdir)/tools/mate-doc-utils.make
	$(GDU_AWK) -f $(srcdir)/mate-doc-make.awk $< > $@

EXTRA_DIST = mate-doc-make.awk

DOC_LINGUAS = de
