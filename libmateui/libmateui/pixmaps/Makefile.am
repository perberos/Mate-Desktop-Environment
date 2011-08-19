# Please update this when you add more `stock_*.png' files
stock_images = \
	stock_attach.png \
	stock_book_blue.png \
	stock_book_green.png \
	stock_book_open.png \
	stock_book_red.png \
	stock_book_yellow.png \
	stock_line_in.png \
	stock_mail.png \
	stock_mail_compose.png \
	stock_mail_forward.png \
	stock_mail_receive.png \
	stock_mail_reply.png \
	stock_mail_send.png \
	stock_menu_about.png \
	stock_menu_blank.png \
	stock_scores.png \
	stock_mic.png \
	stock_multiple_file.png \
	stock_not.png \
	stock_table_borders.png \
	stock_table_fill.png \
	stock_text_bulleted_list.png \
	stock_text_indent.png \
	stock_text_numbered_list.png \
	stock_text_unindent.png \
	stock_timer.png \
	stock_timer_stopped.png \
	stock_trash.png \
	stock_trash_full.png \
	stock_volume.png \
	stock_midi.png \
	stock_authentication.png

EXTRA_DIST = \
	$(stock_images)	\
	copyright.txt

noinst_HEADERS = mate-stock-pixbufs.h
BUILT_SOURCES = mate-stock-pixbufs.h

CLEANFILES = $(BUILT_SOURCES)

mate-stock-pixbufs.h: @GDK_PIXBUF_CSOURCE@ $(stock_images)
	@-rm -f mate-stock-pixbufs.h
	$(AM_V_GEN) var=`echo "$(stock_images)" | sed -e 's,stock_\([^\.]*\)\.png,stock_\1 $(srcdir)/stock_\1.png,g'` ; \
	   @GDK_PIXBUF_CSOURCE@ --raw --build-list $$var > pixbufs-tmp && \
	   cat $(srcdir)/copyright.txt pixbufs-tmp > mate-stock-pixbufs.h
	@-rm -f pixbufs-tmp
