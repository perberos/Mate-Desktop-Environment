## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = 1.6

# The name of the module, e.g. 'glib'.
DOC_MODULE=libmateui

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.sgml

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=--deprecated-guards="MATE_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir).
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR=../../libmateui

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/libmateui/*.h
CFILE_GLOB=$(top_srcdir)/libmateui/*.c

# Header files to ignore when scanning.
IGNORE_HFILES=				\
	mate-i18nP.h			\
	libmateuiP.h			\
	mate-mateconf-ui.h		\
	libmateui-access.h

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
INCLUDES = -I$(top_builddir) -I$(top_srcdir) $(LIBMATEUI_CFLAGS)
GTKDOC_LIBS = $(top_builddir)/libmateui/libmateui-2.la

# Extra options to supply to gtkdoc-mkdb.
MKDB_OPTIONS=--output-format=xml

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =				\
	version.xml

# Images to copy into HTML directory.
HTML_IMAGES =								\
	$(top_srcdir)/libmateui/pixmaps/stock_attach.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_book_blue.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_book_green.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_book_open.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_book_red.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_book_yellow.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_line_in.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mail_compose.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mail_forward.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mail.png			\
	$(top_srcdir)/libmateui/pixmaps/stock_mail_receive.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mail_reply.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mail_send.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_menu_about.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_menu_blank.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_mic.png			\
	$(top_srcdir)/libmateui/pixmaps/stock_midi.png			\
	$(top_srcdir)/libmateui/pixmaps/stock_multiple_file.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_not.png			\
	$(top_srcdir)/libmateui/pixmaps/stock_scores.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_table_borders.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_table_fill.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_text_bulleted_list.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_text_indent.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_text_numbered_list.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_text_unindent.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_timer.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_timer_stopped.png	\
	$(top_srcdir)/libmateui/pixmaps/stock_trash_full.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_trash.png		\
	$(top_srcdir)/libmateui/pixmaps/stock_volume.png

# Extra options to supply to gtkdoc-fixref.
FIXXREF_OPTIONS=

include $(top_srcdir)/gtk-doc.make

# Other files to distribute.
EXTRA_DIST += version.xml.in
