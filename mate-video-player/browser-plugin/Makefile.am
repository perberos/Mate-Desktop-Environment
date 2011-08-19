plugindir = $(BROWSER_PLUGIN_DIR)
plugin_LTLIBRARIES =

EXTRA_DIST =		\
	plugin.symbols	\
	marshal.list	\
	TODO		\
	README.browser-plugin

BUILT_SOURCES =

CLEANFILES = $(BUILT_SOURCES)

# Marshaller

BUILT_SOURCES += \
	marshal.h \
	marshal.c \
	$(NULL)

marshal.h: marshal.list
	$(GLIB_GENMARSHAL) --prefix=totempluginviewer_marshal --internal --header $< > $@
marshal.c: marshal.list
	$(GLIB_GENMARSHAL) --prefix=totempluginviewer_marshal --internal --body --header $< > $@

noinst_LTLIBRARIES = libtotembrowserplugin_helper.la

libtotembrowserplugin_helper_la_SOURCES = \
	marshal.h \
	marshal.c \
	$(NULL)

libtotembrowserplugin_helper_la_CPPFLAGS =	\
	$(DISABLE_DEPRECATED)			\
	$(AM_CPPFLAGS)

libtotembrowserplugin_helper_la_CFLAGS =	\
	$(DEPENDENCY_CFLAGS)		\
	$(WARN_CFLAGS)			\
	$(AM_CFLAGS)

libtotembrowserplugin_helper_la_LIBADD = 	\
	$(DEPENDENCY_CFLAGS)			\
	$(WARN_CFLAGS)				\
	$(AM_CFLAGS)

# The Glow button test program

noinst_PROGRAMS = test-glow-button

test_glow_button_SOURCES = \
	totem-glow-button.c \
	totem-glow-button.h \
	test-glow-button.c

test_glow_button_LDADD = \
	libtotembrowserplugin_helper.la		\
	$(DEPENDENCY_LIBS) -lm

test_glow_button_CFLAGS =	\
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(AM_CFLAGS)

test_glow_button_CPPFLAGS =	\
	$(DISABLE_DEPRECATED)	\
	$(AM_CPPFLAGS)

# Totem Plugin Viewer

libexec_PROGRAMS = totem-plugin-viewer

totem_plugin_viewer_SOURCES = \
	totem-glow-button.c		\
	totem-glow-button.h		\
	totem-plugin-viewer.c		\
	totem-plugin-viewer-interface.h	\
	totem-plugin-viewer-constants.h	\
	totem-plugin-viewer-options.h	\
	$(NULL)

totem_plugin_viewer_CPPFLAGS = \
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/lib				\
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DMATELOCALEDIR="\"$(datadir)/locale\""	\
	-DMATECONF_PREFIX="\"/apps/totem\""		\
	-DDATADIR="\"$(datadir)\""			\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	-DSHAREDIR="\"$(pkgdatadir)\""			\
	-DLOGO_NAME="\"totem_logo.png\""		\
	-DG_LOG_DOMAIN=\""TotemEmbedded\""		\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

totem_plugin_viewer_CFLAGS = \
	$(DEPENDENCY_CFLAGS)	\
	$(WARN_CFLAGS)		\
	$(DBUS_CFLAGS)		\
	$(SN_CFLAGS)		\
	$(AM_CFLAGS)

totem_plugin_viewer_LDADD =						\
	$(top_builddir)/src/backend/libbaconvideowidget.la		\
	$(top_builddir)/src/libtotem_player.la				\
	$(top_builddir)/lib/libtotemscrsaver.la				\
	libtotembrowserplugin_helper.la					\
	$(DEPENDENCY_LIBS)	\
	$(SN_LIBS)		\
	$(XVIDMODE_LIBS) 	\
	$(DBUS_LIBS)		\
	$(XTEST_LIBS)		\
	$(X_LIBS)

BUILT_SOURCES += \
	totem-plugin-viewer-interface.h

totem-plugin-viewer-interface.h: org_mate_totem_PluginViewer.xml
	$(DBUS_BIND) --prefix=totem_embedded --mode=glib-server $< > $@

EXTRA_DIST += \
	org_mate_totem_PluginViewer.xml

# Totem GMP Plugin

if ENABLE_GMP_PLUGIN

plugin_LTLIBRARIES += libtotem-gmp-plugin.la

libtotem_gmp_plugin_la_SOURCES =	\
	npapi.h				\
	npruntime.h			\
	npupp.h				\
	totemNPNGlue.cpp		\
	totemNPClass.cpp		\
	totemNPClass.h			\
	totemNPObject.cpp		\
	totemNPObject.h			\
	totemNPObjectWrapper.h		\
	totemGMPControls.cpp		\
	totemGMPControls.h		\
	totemGMPError.cpp		\
	totemGMPError.h			\
	totemGMPNetwork.cpp		\
	totemGMPNetwork.h		\
	totemGMPPlayer.cpp		\
	totemGMPPlayer.h		\
	totemGMPPlaylist.cpp		\
	totemGMPPlaylist.h		\
	totemGMPSettings.cpp		\
	totemGMPSettings.h		\
	totemPluginGlue.cpp		\
	totemPlugin.cpp			\
	totemPlugin.h			\
	totem-plugin-viewer-constants.h

libtotem_gmp_plugin_la_CPPFLAGS = \
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DTOTEM_GMP_PLUGIN				\
	-DTOTEM_PL_PARSER_MINI				\
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DSYSCONFDIR="\"$(sysconfdir)\""		\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

libtotem_gmp_plugin_la_CXXFLAGS = \
	$(BROWSER_PLUGIN_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(WARN_CXXFLAGS)		\
	$(PLUGIN_EXTRA_WARN_CXXFLAGS)	\
	$(AM_CXXFLAGS)

libtotem_gmp_plugin_la_LIBADD = \
	libtotembrowserplugin_helper.la	\
	$(DBUS_LIBS)			\
	$(BROWSER_PLUGIN_LIBS)

libtotem_gmp_plugin_la_LDFLAGS = \
	-avoid-version	\
	-export-symbols $(srcdir)/plugin.symbols \
	-module		\
	$(AM_LDFLAGS)

endif

# Totem NarrowSpace Plugin

if ENABLE_NARROWSPACE_PLUGIN

plugin_LTLIBRARIES += libtotem-narrowspace-plugin.la

libtotem_narrowspace_plugin_la_SOURCES = \
	npapi.h				\
	npruntime.h			\
	npupp.h				\
	totemNPNGlue.cpp		\
	totemNPClass.cpp		\
	totemNPClass.h			\
	totemNPObject.cpp		\
	totemNPObject.h			\
	totemNPObjectWrapper.h		\
	totemNPVariantWrapper.h		\
	totemPluginGlue.cpp		\
	totemPlugin.cpp			\
	totemPlugin.h			\
	totemNarrowSpacePlugin.cpp	\
	totemNarrowSpacePlugin.h	\
	totem-plugin-viewer-constants.h

libtotem_narrowspace_plugin_la_CPPFLAGS = \
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DTOTEM_NARROWSPACE_PLUGIN			\
	-DTOTEM_PL_PARSER_MINI				\
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DSYSCONFDIR="\"$(sysconfdir)\""		\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

libtotem_narrowspace_plugin_la_CXXFLAGS = \
	$(BROWSER_PLUGIN_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(WARN_CXXFLAGS)		\
	$(PLUGIN_EXTRA_WARN_CXXFLAGS)	\
	$(AM_CXXFLAGS)

libtotem_narrowspace_plugin_la_LIBADD = \
	libtotembrowserplugin_helper.la	\
	$(DBUS_LIBS)			\
	$(BROWSER_PLUGIN_LIBS)

libtotem_narrowspace_plugin_la_LDFLAGS = \
	-avoid-version	\
	-export-symbols $(srcdir)/plugin.symbols \
	-module		\
	$(AM_LDFLAGS)

endif

# Totem MULLY Plugin

if ENABLE_MULLY_PLUGIN

plugin_LTLIBRARIES += libtotem-mully-plugin.la

libtotem_mully_plugin_la_SOURCES =	\
	npapi.h				\
	npruntime.h			\
	npupp.h				\
	totemNPNGlue.cpp		\
	totemNPClass.cpp		\
	totemNPClass.h			\
	totemNPObject.cpp		\
	totemNPObject.h			\
	totemNPObjectWrapper.h		\
	totemNPVariantWrapper.h		\
	totemPluginGlue.cpp		\
	totemPlugin.cpp			\
	totemPlugin.h			\
	totemMullYPlugin.cpp		\
	totemMullYPlugin.h		\
	totem-plugin-viewer-constants.h

libtotem_mully_plugin_la_CPPFLAGS = \
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DTOTEM_MULLY_PLUGIN				\
	-DTOTEM_PL_PARSER_MINI				\
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DSYSCONFDIR="\"$(sysconfdir)\""		\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

libtotem_mully_plugin_la_CXXFLAGS = \
	$(BROWSER_PLUGIN_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(WARN_CXXFLAGS)		\
	$(PLUGIN_EXTRA_WARN_CXXFLAGS)	\
	$(AM_CXXFLAGS)

libtotem_mully_plugin_la_LIBADD = \
	libtotembrowserplugin_helper.la	\
	$(DBUS_LIBS)			\
	$(BROWSER_PLUGIN_LIBS)

libtotem_mully_plugin_la_LDFLAGS = \
	-avoid-version	\
	-export-symbols $(srcdir)/plugin.symbols \
	-module		\
	$(AM_LDFLAGS)

endif

# Totem Cone Plugin

if ENABLE_CONE_PLUGIN

plugin_LTLIBRARIES += libtotem-cone-plugin.la

libtotem_cone_plugin_la_SOURCES =	\
	npapi.h				\
	npruntime.h			\
	npupp.h				\
	totemNPNGlue.cpp		\
	totemNPClass.cpp		\
	totemNPClass.h			\
	totemNPObject.cpp		\
	totemNPObject.h			\
	totemNPObjectWrapper.h		\
	totemNPVariantWrapper.h		\
	totemConeAudio.cpp		\
	totemConeAudio.h		\
	totemCone.cpp			\
	totemCone.h			\
	totemConeInput.cpp		\
	totemConeInput.h		\
	totemConePlaylist.cpp		\
	totemConePlaylist.h		\
	totemConePlaylistItems.cpp	\
	totemConePlaylistItems.h	\
	totemConeVideo.cpp		\
	totemConeVideo.h		\
	totemPluginGlue.cpp		\
	totemPlugin.cpp			\
	totemPlugin.h			\
	totem-plugin-viewer-constants.h

libtotem_cone_plugin_la_CPPFLAGS = \
	-I$(top_srcdir)/src				\
	-I$(top_srcdir)/src/backend			\
	-I$(top_builddir)/src				\
	-I$(top_builddir)/src/backend			\
	-DXP_UNIX					\
	-DMOZ_X11					\
	-DTOTEM_CONE_PLUGIN				\
	-DTOTEM_PL_PARSER_MINI				\
	-D_REENTRANT					\
	-DDBUS_API_SUBJECT_TO_CHANGE			\
	-DSYSCONFDIR="\"$(sysconfdir)\""		\
	-DLIBEXECDIR="\"$(libexecdir)\""		\
	$(DISABLE_DEPRECATED)				\
	$(AM_CPPFLAGS)

libtotem_cone_plugin_la_CXXFLAGS = \
	$(BROWSER_PLUGIN_CFLAGS)	\
	$(DBUS_CFLAGS)			\
	$(WARN_CXXFLAGS)		\
	$(PLUGIN_EXTRA_WARN_CXXFLAGS)	\
	$(AM_CXXFLAGS)

libtotem_cone_plugin_la_LIBADD = \
	libtotembrowserplugin_helper.la	\
	$(DBUS_LIBS)			\
	$(BROWSER_PLUGIN_LIBS)

libtotem_cone_plugin_la_LDFLAGS = \
	-avoid-version	\
	-export-symbols $(srcdir)/plugin.symbols \
	-module		\
	$(AM_LDFLAGS)

endif
