EXTRA_DIST = $(GKB_MAPS) $(xmm_xml_in_file)

gkbmapsdir = $(datadir)/xmodmap
gkbmaps_DATA = $(GKB_MAPS) $(XMM_XML)

GKB_MAPS = \
		xmodmap.am \
		xmodmap.az \
		xmodmap.be \
		xmodmap.bg \
		xmodmap.br \
		xmodmap.by \
		xmodmap.ca \
		xmodmap.cz \
		xmodmap.de \
		xmodmap.de_CH \
		xmodmap.dk \
		xmodmap.ee \
		xmodmap.es \
		xmodmap.fi \
		xmodmap.fr \
		xmodmap.gb \
		xmodmap.hu \
		xmodmap.hu-lat1 \
		xmodmap.il \
		xmodmap.is \
		xmodmap.it \
		xmodmap.jp \
		xmodmap.lt \
		xmodmap.lv \
		xmodmap.mk \
		xmodmap.mn \
		xmodmap.no \
		xmodmap.pl \
		xmodmap.pt \
		xmodmap.ro \
		xmodmap.ru \
		xmodmap.se \
		xmodmap.si \
		xmodmap.sk \
		xmodmap.sr \
		xmodmap.th \
		xmodmap.tr \
		xmodmap.ua \
		xmodmap.us \
		xmodmap.vn \
		xmodmap.br-usb \
		xmodmap.de-usb \
		xmodmap.dk-usb \
		xmodmap.es-usb \
		xmodmap.fr-usb \
		xmodmap.gb-usb \
		xmodmap.it-usb \
		xmodmap.se-usb \
		xmodmap.us-usb \
		xmodmap.de-type4 \
		xmodmap.dk-type4 \
		xmodmap.es-type4 \
		xmodmap.gb-type4 \
		xmodmap.it-type4 \
		xmodmap.jp-type4 \
		xmodmap.pt-type4 \
		xmodmap.se-type4 \
		xmodmap.us-type4 \
		xmodmap.us-type6

xmm_xml_in_file = base.xml.in
XMM_XML = $(xmm_xml_in_file:.xml.in=.xml)

%.xml: %.xml.in $(INTLTOOL_MERGE) $(wildcard $(top_srcdir)/po/*.po)
		$(INTLTOOL_MERGE) $(top_srcdir)/po $< $@ -x -u \
		-c $(top_srcdir)/po/.intltool-merge-cache

-include $(top_srcdir)/git.mk
