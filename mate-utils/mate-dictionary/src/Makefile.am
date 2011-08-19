NULL =

INCLUDES = \
	-DPREFIX=\""$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DLIBDIR=\""$(libdir)"\"			\
	-DDATADIR=\""$(datadir)"\" 			\
	-DUIDATADIR=\""$(datadir)/mate-2.0/ui"\" 	\
	-DPKGDATADIR=\""$(datadir)/mate-dictionary"\" 	\
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(NULL)

bin_PROGRAMS = mate-dictionary

if BUILD_GDICT_APPLET
libexec_PROGRAMS = mate-dictionary-applet
endif

mate_dictionary_SOURCES = \
	gdict-about.c		\
	gdict-about.h		\
	gdict-app.c		\
	gdict-app.h		\
	gdict-common.c 		\
	gdict-common.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	gdict-print.c 		\
	gdict-print.h 		\
	gdict-sidebar.c		\
	gdict-sidebar.h		\
	gdict-source-dialog.c 	\
	gdict-source-dialog.h 	\
	gdict-window.c 		\
	gdict-window.h 		\
	main.c			\
	$(NULL)
mate_dictionary_CFLAGS = \
	-I$(top_builddir)/mate-dictionary \
	-I$(top_srcdir)/mate-dictionary \
	$(MATE_UTILS_CFLAGS) \
	$(NULL)
mate_dictionary_LDADD = \
	-lm			\
	$(top_builddir)/mate-dictionary/libgdict/libgdict-1.0.la \
	$(MATE_UTILS_LIBS) \
	$(NULL)

if BUILD_GDICT_APPLET

mate_dictionary_applet_SOURCES = \
	gdict-about.c 		\
	gdict-about.h		\
	gdict-aligned-window.c	\
	gdict-aligned-window.h	\
	gdict-applet.c		\
	gdict-applet.h		\
	gdict-common.c		\
	gdict-common.h		\
	gdict-pref-dialog.c	\
	gdict-pref-dialog.h	\
	gdict-print.c 		\
	gdict-print.h 		\
	gdict-sidebar.c		\
	gdict-sidebar.h		\
	gdict-source-dialog.c 	\
	gdict-source-dialog.h 	\
	$(NULL)
mate_dictionary_applet_CFLAGS = \
	-I$(top_builddir)/mate-dictionary \
	-I$(top_srcdir)/mate-dictionary \
	$(MATE_UTILS_CFLAGS) \
	$(APPLET_CFLAGS) \
	$(NULL)
mate_dictionary_applet_LDADD = \
	-lm			\
	$(top_builddir)/mate-dictionary/libgdict/libgdict-1.0.la \
	$(MATE_UTILS_LIBS) \
	$(APPLET_LIBS) \
	$(NULL)

endif # BUILD_GDICT_APPLET
