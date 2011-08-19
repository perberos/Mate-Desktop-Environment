EXTRA_DIST = $(GKB_MAPS) $(xmm_xml_in_file)

gkbmapsdir = $(datadir)/xmodmap
gkbmaps_DATA = $(GKB_MAPS) $(XMM_XML)

GKB_MAPS = \
	xmodmap.am xmodmap.be xmodmap.bg xmodmap.br xmodmap.ch xmodmap.ch_de \
	xmodmap.ch_fr xmodmap.cz xmodmap.de xmodmap.de-apple xmodmap.dk \
	xmodmap.dvorak xmodmap.ee xmodmap.es xmodmap.fi xmodmap.fr \
	xmodmap.fr-2 xmodmap.gb xmodmap.gb-102 xmodmap.gb-105 xmodmap.ge_la \
	xmodmap.ge_ru xmodmap.gr xmodmap.hu xmodmap.hu-101-lat1 \
	xmodmap.hu-101-lat2 xmodmap.hu-ibm xmodmap.hu-lat1 \
	xmodmap.hu-sun-lat2 xmodmap.hu.old xmodmap.hu_latin1 xmodmap.il \
	xmodmap.il_phonetic xmodmap.is xmodmap.it xmodmap.jp xmodmap.la \
	xmodmap.lt xmodmap.lt_b xmodmap.lt_p xmodmap.nl xmodmap.no \
	xmodmap.pl xmodmap.pl2 xmodmap.pt xmodmap.pt-dead xmodmap.qc \
	xmodmap.qc-2 xmodmap.ru xmodmap.ru_yawerty xmodmap.se xmodmap.sf \
	xmodmap.sg xmodmap.si xmodmap.sk xmodmap.th xmodmap.tr_f \
	xmodmap.tr_q xmodmap.uk xmodmap.us xmodmap.us-101 xmodmap.us-84 \
	xmodmap.us-dec xmodmap.us-ibm xmodmap.us-int xmodmap.us-mac \
	xmodmap.us-sgi-101 xmodmap.us-sun xmodmap.us.old xmodmap.us_intl \
	xmodmap.yu xmodmap.ro xmodmap.mk xmodmap.ru-rev xmodmap.es_x86 \
	xmodmap.fr_x86 xmodmap.uk_x86 xmodmap.us101A_x86 xmodmap.hu_x86 \
	xmodmap.ar xmodmap.kr xmodmap.mn-rev xmodmap.mn xmodmap.mn-phonet \
	xmodmap.mn-uni

xmm_xml_in_file = base.xml.in
XMM_XML = $(xmm_xml_in_file:.xml.in=.xml)

%.xml: %.xml.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)
		$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u \
		-c $(top_srcdir)/po/.intltool-merge-cache

CLEANFILES = base.xml

-include $(top_srcdir)/git.mk
