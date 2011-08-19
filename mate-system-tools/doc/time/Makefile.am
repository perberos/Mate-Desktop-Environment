include $(top_srcdir)/mate-doc-utils.make
dist-hook: doc-dist-hook

DOC_MODULE = time-admin
DOC_ENTITIES = legal.xml
DOC_INCLUDES = 
DOC_FIGURES = \
	figures/time-map.png		\
	figures/time-servers.png	\
	figures/time-tool-automatic.png	\
	figures/time-tool-manual.png

DOC_LINGUAS = ca cs de el en_GB es fr gl it oc pt_BR ru sv zh_CN

# This isn't still ported to mate-doc-utils
SUBDIRS = nl

-include $(top_srcdir)/git.mk
