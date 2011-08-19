NULL =

bin_PROGRAMS = 					\
	mate-volume-control-applet		\
	mate-volume-control			\
	$(NULL)

AM_CPPFLAGS =					\
	$(WARN_CFLAGS)				\
	-I$(top_srcdir)/sound-theme		\
	$(VOLUME_CONTROL_CFLAGS)		\
	$(DISABLE_DEPRECATED)			\
	$(PULSEAUDIO_CFLAGS)			\
	-DLOCALE_DIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DGLADEDIR=\""$(pkgdatadir)"\"		\
	-DICON_DATA_DIR="\"$(pkgdatadir)/icons\"" \
	$(NULL)

noinst_LTLIBRARIES = libmatevolumecontrol.la
libmatevolumecontrol_la_SOURCES =		\
	gvc-mixer-card.h			\
	gvc-mixer-card.c			\
	gvc-mixer-stream.h			\
	gvc-mixer-stream.c			\
	gvc-channel-map.h			\
	gvc-channel-map.c			\
	gvc-mixer-sink.h			\
	gvc-mixer-sink.c			\
	gvc-mixer-source.h			\
	gvc-mixer-source.c			\
	gvc-mixer-sink-input.h			\
	gvc-mixer-sink-input.c			\
	gvc-mixer-source-output.h		\
	gvc-mixer-source-output.c		\
	gvc-mixer-event-role.h			\
	gvc-mixer-event-role.c			\
	gvc-mixer-control.h			\
	gvc-mixer-control.c			\
	gvc-channel-bar.h			\
	gvc-channel-bar.c			\
	gvc-log.h				\
	gvc-log.c				\
	$(NULL)

mate_volume_control_applet_LDADD =		\
	-lm					\
	libmatevolumecontrol.la		\
	$(VOLUME_CONTROL_LIBS)			\
	$(PULSEAUDIO_LIBS)			\
	$(NULL)

mate_volume_control_applet_SOURCES =		\
	gvc-stream-status-icon.h		\
	gvc-stream-status-icon.c		\
	gvc-applet.h				\
	gvc-applet.c				\
	applet-main.c				\
	$(NULL)

mate_volume_control_LDADD =				\
	-lm						\
	libmatevolumecontrol.la			\
	$(top_builddir)/sound-theme/libsoundtheme.la	\
	$(VOLUME_CONTROL_LIBS)				\
	$(PULSEAUDIO_LIBS)				\
	$(NULL)

mate_volume_control_SOURCES =			\
	gvc-balance-bar.h			\
	gvc-balance-bar.c			\
	gvc-mixer-dialog.h			\
	gvc-mixer-dialog.c			\
	gvc-level-bar.h				\
	gvc-level-bar.c				\
	gvc-combo-box.h				\
	gvc-combo-box.c				\
	gvc-speaker-test.h			\
	gvc-speaker-test.c			\
	dialog-main.c				\
	$(NULL)

BUILT_SOURCES =				\
	$(NULL)

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

-include $(top_srcdir)/git.mk
