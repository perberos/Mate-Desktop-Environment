SUBDIRS = plugins backend

bin_PROGRAMS = totem totem-video-thumbnailer totem-video-indexer totem-audio-preview
libexec_PROGRAMS =
noinst_LTLIBRARIES =				\
	libtotem_player.la			\
	libtotem_main.la

common_defines = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DMATELOCALEDIR=\""$(datadir)/locale"\"	\
	-DMATECONF_PREFIX=\""/apps/totem"\"		\
	-DDATADIR=\""$(datadir)"\"			\
	-DLIBEXECDIR=\""$(libexecdir)"\"		\
	-DBINDIR=\""$(bindir)"\"			\
	-DGTKBUILDERDIR=\"""\"				\
	$(DISABLE_DEPRECATED)

modules_flags = -export_dynamic -avoid-version -module -no-undefined

# Totem UI ltlibrary (used by browser plugins)

libtotem_player_la_SOURCES = \
	totem-statusbar.c	\
	totem-statusbar.h	\
	totem-interface.c	\
	totem-interface.h	\
	totem-fullscreen.c	\
	totem-fullscreen.h	\
	gsd-media-keys-window.c	\
	gsd-media-keys-window.h	\
	totem-time-label.c	\
	totem-time-label.h

libtotem_player_la_CPPFLAGS = \
	-I$(srcdir)/backend		\
	-I$(top_srcdir)/		\
	$(common_defines)		\
	-DG_LOG_DOMAIN=\""Totem"\"	\
	$(AM_CPPFLAGS)

libtotem_player_la_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(UNIQUE_CFLAGS)	\
	$(AM_CFLAGS)

libtotem_player_la_LDFLAGS = \
	$(AM_LDFLAGS) 

# Totem main library (used for main player; separate to allow gtk-doc to be used)

libtotem_main_la_SOURCES = \
	$(TOTEMMARSHALFILES)		\
	totem-object.c			\
	totem.h				\
	totem-private.h			\
	totem-cell-renderer-video.c	\
	totem-cell-renderer-video.h	\
	totem-video-list.c		\
	totem-video-list.h		\
	totem-preferences.c		\
	totem-preferences.h		\
	totem-dnd-menu.c		\
	totem-dnd-menu.h 		\
	totem-options.c			\
	totem-options.h			\
	totem-playlist.c		\
	totem-playlist.h		\
	eggfileformatchooser.c		\
	eggfileformatchooser.h		\
	egg-macros.h			\
	totem-session.c			\
	totem-session.h			\
	totem-sidebar.c			\
	totem-sidebar.h 		\
	totem-open-location.c		\
	totem-open-location.h 		\
	totem-menu.c			\
	totem-menu.h			\
	totem-uri.c			\
	totem-uri.h			\
	ev-sidebar.c			\
	ev-sidebar.h			\
	totem-subtitle-encoding.c	\
	totem-subtitle-encoding.h	\
	totem-profile.h

libtotem_main_la_CPPFLAGS = \
	-I$(top_srcdir)/		\
	-I$(srcdir)/backend		\
	-I$(srcdir)/plugins		\
	-I$(top_builddir)/data		\
	$(common_defines)		\
	-DG_LOG_DOMAIN=\""Totem"\"	\
	$(AM_CPPFLAGS)

libtotem_main_la_CFLAGS = \
	$(DEPENDENCY_CFLAGS)		\
	$(WARN_CFLAGS)			\
	$(DBUS_CFLAGS)			\
	$(MISSING_PLUGINS_CFLAGS)	\
	$(UNIQUE_CFLAGS)		\
	$(AM_CFLAGS)

libtotem_main_la_LDFLAGS = \
	$(AM_LDFLAGS)

libtotem_main_la_LIBADD = \
	libtotem_player.la		\
	backend/libbaconvideowidget.la	\
	plugins/libtotemmodule.la	\
	$(UNIQUE_LIBS)			\
	$(DBUS_LIBS)			\
	$(XVIDMODE_LIBS)		\
	$(XTEST_LIBS)			\
	$(X_LIBS)

if WITH_SMCLIENT
libtotem_main_la_SOURCES += \
	eggsmclient.c \
	eggsmclient.h \
	eggsmclient-private.h \
	$(NULL)
libtotem_main_la_CFLAGS += $(SMCLIENT_CFLAGS)
libtotem_main_la_LIBADD += $(SMCLIENT_LIBS)

if WITH_SMCLIENT_XSMP
libtotem_main_la_SOURCES += \
	eggdesktopfile.c \
	eggdesktopfile.h \
	eggsmclient-xsmp.c
libtotem_main_la_CPPFLAGS += -DEGG_SM_CLIENT_BACKEND_XSMP
endif
if WITH_SMCLIENT_WIN32
libtotem_main_la_SOURCES += eggsmclient-win32.c
endif
if WITH_SMCLIENT_QUARTZ
libtotem_main_la_SOURCES += eggsmclient-osx.c
endif
endif

if ENABLE_PYTHON
libtotem_main_la_LIBADD += \
	$(PYTHON_LIBS)						\
	$(top_builddir)/bindings/python/totem-python.la
endif

# Totem

TOTEMMARSHALFILES = 						\
	totemobject-marshal.c totemobject-marshal.h		\
	totemplaylist-marshal.c totemplaylist-marshal.h		\
	totemvideolist-marshal.c totemvideolist-marshal.h
GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
BUILT_SOURCES = $(TOTEMMARSHALFILES)

totemobject-marshal.h: totemobject-marshal.list Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemobject_marshal $(srcdir)/totemobject-marshal.list --header > totemobject-marshal.h )
totemobject-marshal.c: totemobject-marshal.h Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemobject_marshal $(srcdir)/totemobject-marshal.list --header --body > totemobject-marshal.c )
totemplaylist-marshal.h: totemplaylist-marshal.list Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemplaylist_marshal $(srcdir)/totemplaylist-marshal.list --header > totemplaylist-marshal.h )
totemplaylist-marshal.c: totemplaylist-marshal.h Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemplaylist_marshal $(srcdir)/totemplaylist-marshal.list --header --body > totemplaylist-marshal.c )
totemvideolist-marshal.h: totemvideolist-marshal.list Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemvideolist_marshal $(srcdir)/totemvideolist-marshal.list --header > totemvideolist-marshal.h )
totemvideolist-marshal.c: totemvideolist-marshal.h Makefile
	$(AM_V_GEN) ( $(GLIB_GENMARSHAL) --prefix=totemvideolist_marshal $(srcdir)/totemvideolist-marshal.list --header --body > totemvideolist-marshal.c )

totem_SOURCES = totem.c

totem_CPPFLAGS =			\
	-I$(top_srcdir)/		\
	-I$(srcdir)/backend		\
	$(common_defines)		\
	-DG_LOG_DOMAIN=\""Totem"\"	\
	$(AM_CPPFLAGS)

totem_CFLAGS =				\
	$(UNIQUE_CFLAGS)		\
	$(WARN_CFLAGS)			\
	$(DEPENDENCY_CFLAGS)		\
	$(AM_CFLAGS)

totem_LDFLAGS = \
	$(AM_LDFLAGS)

totem_LDADD =						\
	libtotem_main.la				\
	$(DEPENDENCY_LIBS)				\
	$(XVIDMODE_LIBS)				\
	$(XTEST_LIBS)					\
	$(X_LIBS)

# Totem video thumbnailer

totem_video_thumbnailer_SOURCES =	\
	totem-video-thumbnailer.c	\
	totem-resources.c		\
	totem-resources.h

totem_video_thumbnailer_CPPFLAGS = \
	-I$(top_srcdir)/				\
	-I$(srcdir)/backend				\
	$(common_defines)				\
	-DG_LOG_DOMAIN=\""TotemVideoThumbnailer"\"	\
	$(AM_CPPFLAGS)

totem_video_thumbnailer_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

totem_video_thumbnailer_LDFLAGS = \
	$(AM_LDFLAGS)

totem_video_thumbnailer_LDADD =			\
	backend/libbaconvideowidget.la		\
	$(DEPENDENCY_LIBS)			\
	$(EXTRA_BACKEND_LIBS) 			\
	$(XTEST_LIBS)				\
	$(XVIDMODE_LIBS)			\
	$(X_LIBS)

# Caja Property Page

if HAVE_CAJA

cajadir = $(CAJADIR)
caja_LTLIBRARIES = libtotem-properties-page.la

libtotem_properties_page_la_SOURCES = 	\
	totem-properties-main.c		\
	totem-properties-view.c		\
	totem-properties-view.h

libtotem_properties_page_la_CPPFLAGS =		\
	-I$(srcdir)/backend			\
	-I$(top_srcdir)/			\
	-I$(srcdir)/plugins/properties		\
	-I$(top_builddir)/data			\
	$(common_defines)			\
	-DG_LOG_DOMAIN=\""TotemPropertiesPage"\"\
	$(AM_CPPFLAGS)

libtotem_properties_page_la_CFLAGS = \
	$(WARN_CFLAGS)		\
	$(DEPENDENCY_CFLAGS)	\
	$(CAJA_CFLAGS)	\
	$(AM_CFLAGS)

libtotem_properties_page_la_LDFLAGS = \
	$(modules_flags)	\
	$(AM_LDFLAGS)

libtotem_properties_page_la_LIBADD =				\
	backend/libbaconvideowidget.la				\
	plugins/properties/libbaconvideowidgetproperties.la	\
	libtotem_player.la					\
	$(DEPENDENCY_LIBS)					\
	$(CAJA_LIBS)					\
	$(XTEST_LIBS)						\
	$(XVIDMODE_LIBS)					\
	$(X_LIBS)

noinst_PROGRAMS = test-properties-page

test_properties_page_SOURCES =		\
	totem-properties-main.c		\
	totem-properties-view.c		\
	totem-properties-view.h		\
	test-properties-page.c

test_properties_page_CPPFLAGS = 		\
	-I$(srcdir)/backend			\
	-I$(top_srcdir)/			\
	-I$(top_builddir)/data			\
	-I$(srcdir)/plugins/properties		\
	$(common_defines)			\
	$(AM_CPPFLAGS)

test_properties_page_CFLAGS = \
	$(WARN_CFLAGS)		\
	$(DEPENDENCY_CFLAGS)	\
	$(CAJA_CFLAGS)	\
	$(AM_CFLAGS)

test_properties_page_LDFLAGS = \
	$(AM_LDFLAGS)

test_properties_page_LDADD = 					\
	backend/libbaconvideowidget.la				\
	plugins/properties/libbaconvideowidgetproperties.la	\
	libtotem_player.la					\
	$(DEPENDENCY_LIBS)					\
	$(CAJA_LIBS)					\
	$(XTEST_LIBS)						\
	$(XVIDMODE_LIBS)					\
	$(X_LIBS)

endif # HAVE_CAJA

# Totem Video Indexer

totem_video_indexer_SOURCES =		\
	totem-video-indexer.c		\
	totem-resources.c		\
	totem-resources.h

totem_video_indexer_CPPFLAGS = \
	-I$(top_srcdir)/			\
	-I$(srcdir)/backend			\
	-I$(top_builddir)/data			\
	$(common_defines)			\
	-DG_LOG_DOMAIN=\""TotemVideoIndexer"\"	\
	$(AM_CPPFLAGS)

totem_video_indexer_CFLAGS = \
	$(WARN_CFLAGS)		\
	$(DEPENDENCY_CFLAGS)	\
	$(AM_CFLAGS)

totem_video_indexer_LDFLAGS = \
	$(AM_LDFLAGS)

totem_video_indexer_LDADD = \
	backend/libbaconvideowidget.la	\
	$(DEPENDENCY_LIBS)		\
	$(EXTRA_BACKEND_LIBS)		\
	$(XTEST_LIBS)			\
	$(XVIDMODE_LIBS)		\
	$(X_LIBS)

# Totem Audio Preview for caja
totem_audio_preview_SOURCES =		\
	totem-audio-preview.c		\
	totem-resources.c		\
	totem-resources.h

totem_audio_preview_CPPFLAGS =	\
	-I$(top_srcdir)/			\
	-I$(srcdir)/backend			\
	-I$(top_builddir)/data			\
	$(common_defines)			\
	-DG_LOG_DOMAIN=\""TotemAudioPreview"\"	\
	$(AM_CPPFLAGS)

totem_audio_preview_CFLAGS = \
	$(WARN_CFLAGS)		\
	$(DEPENDENCY_CFLAGS)	\
	$(AM_CFLAGS)

totem_audio_preview_LDFLAGS = \
	$(AM_LDFLAGS)

totem_audio_preview_LDADD = \
	backend/libbaconvideowidget.la	\
	$(DEPENDENCY_LIBS)		\
	$(EXTRA_BACKEND_LIBS)		\
	$(XTEST_LIBS)			\
	$(XVIDMODE_LIBS)		\
	$(X_LIBS)

CLEANFILES =			\
	*.bak			\
	core*			\
	*.orig			\
	*~			\
	$(desktop_DATA)		\
	$(BUILT_SOURCES)

EXTRA_DIST = 				\
	totemobject-marshal.list	\
	totemvideolist-marshal.list	\
	totemplaylist-marshal.list
		
