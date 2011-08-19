## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/grecord/src/recent-files/		\
	$(GSR_CFLAGS)						\
	$(DISABLE_DEPRECATED)					\
	-DDATADIR=\""$(datadir)"\"				\
	-DGSR_DATADIR=\""$(datadir)/mate-sound-recorder"\"	\
	-DGSR_UIDIR=\""$(datadir)/mate-sound-recorder/ui/"\"

bin_PROGRAMS = mate-sound-recorder

mate_sound_recorder_SOURCES =	\
	mate-recorder.c	\
	gsr-window.c		\
	gsr-window.h

uidir = $(datadir)/mate-sound-recorder/ui
ui_DATA = ui.xml

EXTRA_DIST = $(ui_DATA)

mate_sound_recorder_LDADD =					\
	-lm							\
	$(GSR_LIBS) -lgstinterfaces-@GST_MAJORMINOR@		\
	$(top_builddir)/profiles/libmate-media-profiles.la

-include $(top_srcdir)/git.mk
