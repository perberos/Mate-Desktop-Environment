include $(top_srcdir)/tools/mate-doc-utils.make
include $(top_srcdir)/bootstrap.make
dist-hook: doc-dist-hook

ABS_SRCDIR = @abs_srcdir@

xsldoc_dirs = 					\
	$(ABS_SRCDIR)/../../xslt/common		\
	$(ABS_SRCDIR)/../../xslt/gettext		\
	$(ABS_SRCDIR)/../../xslt/docbook/common	\
	$(ABS_SRCDIR)/../../xslt/docbook/html		\
	$(ABS_SRCDIR)/../../xslt/docbook/omf
xsldoc_xsls = $(filter-out %db2html-suppressed.xsl,			\
	$(sort $(patsubst ./%, %,					\
	  $(foreach dir,$(xsldoc_dirs), $(wildcard $(dir)/*.xsl)) )) )
xsldoc_base = $(sort $(foreach xsl,$(xsldoc_xsls), $(basename $(notdir $(xsl)))))
xsldoc_docs = $(patsubst %, C/%.xsldoc, $(xsldoc_base))
xsldoc_xmls = $(patsubst %, C/%.xml, $(xsldoc_base))

all: $(xsldoc_docs) $(xsldoc_xmls)

$(xsldoc_docs): $(xsldoc_xsls) xsldoc.awk xsldoc-fill.xsl
	$(GDU_AWK) -f "$(srcdir)/xsldoc.awk" "$(filter %/$(basename $(notdir $@)).xsl,$(xsldoc_xsls))" \
	| xsltproc -o "$@" \
	--stringparam basename "$(basename $(notdir $@))" \
	--stringparam xsl_file "$(filter %/$(basename $(notdir $@)).xsl,$(xsldoc_xsls))" \
	"$(srcdir)/xsldoc-fill.xsl" -

$(xsldoc_xmls): xsldoc-docbook.xsl
$(xsldoc_xmls): C/%.xml : C/%.xsldoc
	xsltproc -o "$@" \
	--stringparam basename "$(basename $(notdir $@))" \
	--stringparam xsl_file "$(filter %/$(basename $(notdir $@)).xsl,$(xsldoc_xsls))" \
	"$(srcdir)/xsldoc-docbook.xsl" "$<"

mate_doc_xslt_includes='\
/<\!--BEGIN-->/ { inc=1; }\
/<\!--END-->/ { inc=0; }\
inc {\
  if (match($$0, /<include href="[^"]*\.xml"/)) {\
    base = $$0;\
    sub(/.*<include href="/, "", base);\
    sub(/\.xml".*/, "", base);\
    print base;\
  }\
}'

.PHONY: mate-doc-xslt-check-includes mate-doc-xslt-check-xsldoc
all: mate-doc-xslt-check-includes mate-doc-xslt-check-xsldoc
mate-doc-xslt-check-includes:
	@incs=`$(GDU_AWK) $(mate_doc_xslt_includes) "$(srcdir)/C/mate-doc-xslt.xml" | sort`; \
	xmls=`echo $(xsldoc_base) | sort`; \
	exs=""; \
	for xml in $$xmls; do \
	  if echo "$$incs" | tr '\n' '\ ' | grep -v "\<$$xml\>" 1>/dev/null; then \
	    exs="$$exs$$xml "; \
	fi; done; \
	if [ "x$$exs" != "x" ]; then \
	  echo "mate-doc-xslt.xml does not include the following reference pages: $$exs" 1>&2; \
	  exit 1; \
	fi
mate-doc-xslt-check-xsldoc: $(xsldoc_docs)
	@echo "Running xsldoc checks";
	@(echo "<xsldoc>"; cat $(xsldoc_docs); echo "</xsldoc>") \
	| xsltproc "$(srcdir)/xsldoc-check.xsl" - 1> /dev/null

.PHONY: clean-xsldoc
clean-local: clean_xsldoc
clean-doc-dir: clean_xsldoc
clean_xsldoc:
	rm -f $(xsldoc_docs) $(xsldoc_xmls)

EXTRA_DIST = xsldoc.awk xsldoc-fill.xsl xsldoc-check.xsl xsldoc-docbook.xsl

DOC_MODULE = mate-doc-xslt
DOC_INCLUDES = legal.xml translating.xml $(patsubst %,%.xml,$(xsldoc_base))

DOC_LINGUAS = de
