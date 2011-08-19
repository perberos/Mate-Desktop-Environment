bin_PROGRAMS = matedialog

bin_SCRIPTS = gdialog

matedialog_SOURCES = \
	about.c		\
	calendar.c	\
	entry.c		\
	fileselection.c	\
	main.c		\
	msg.c		\
	notification.c	\
	option.c	\
	option.h	\
	progress.c	\
	scale.c		\
	text.c		\
	tree.c		\
	color.c         \
	password.c      \
	util.c		\
	util.h		\
	matedialog.h

matedialog_CPPFLAGS = \
	-I$(includedir)					\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATEDIALOG_DATADIR=\""$(pkgdatadir)"\"		\
	$(AM_CPPFLAGS)

matedialog_CFLAGS = \
	$(MATEDIALOG_CFLAGS)	\
	$(LIBMATENOTIFY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

matedialog_LDFLAGS = \
	$(AM_LDFLAGS)

matedialog_LDADD = \
	$(MATEDIALOG_LIBS)		\
	$(LIBMATENOTIFY_LIBS)	

uidir = $(datadir)/matedialog

ui_DATA = \
	matedialog.ui

DISTCLEANFILES= \
	gdialog

EXTRA_DIST = \
	$(ui_DATA) \
	gdialog \
	gdialog.in
