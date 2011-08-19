bin_PROGRAMS = mate-audio-profiles-properties
noinst_PROGRAMS =

defines = \
	-DG_LOG_DOMAIN=\"mate-media-profiles\" \
	-DDATADIR="\"$(pkgdatadir)\"" \
	-DGMP_UIDIR=\"$(uidir)\" \
	-DMATELOCALEDIR=\""$(datadir)/locale"\" \
	$(DISABLE_DEPRECATED)

if HAVE_GSTREAMER
noinst_PROGRAMS += mate-audio-profiles-test
mate_audio_profiles_test_SOURCES = mate-audio-profiles-test.c
mate_audio_profiles_test_CFLAGS = $(GMP_CFLAGS) $(GSR_CFLAGS) $(defines) -I$(top_srcdir)
mate_audio_profiles_test_LDADD = libmate-media-profiles.la $(GMP_LIBS) $(GSR_LIBS)
endif

lib_LTLIBRARIES = libmate-media-profiles.la

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

uidir = $(pkgdatadir)
ui_DATA = mate-audio-profile-new.ui mate-audio-profile-edit.ui

mate_audio_profiles_properties_SOURCES = mate-audio-profiles-properties.c
mate_audio_profiles_properties_CFLAGS = $(GMP_CFLAGS) $(defines) -I$(top_srcdir)
mate_audio_profiles_properties_LDADD = libmate-media-profiles.la $(GMP_LIBS)

libmate_media_profiles_la_SOURCES = \
	audio-profile.c \
	audio-profiles-edit.c \
	audio-profile-edit.c \
	audio-profile-choose.c \
	mate-media-profiles.c \
	gmp-util.c

noinst_HEADERS = \
	audio-profile-private.h \
	gmp-conf.h \
	gmp-util.h


gmpincludedir = $(includedir)/mate-media/profiles
gmpinclude_HEADERS = \
	audio-profile.h \
	audio-profile-choose.h \
	audio-profile-edit.h \
	audio-profiles-edit.h \
	mate-media-profiles.h

libmate_media_profiles_la_CFLAGS = $(GMP_CFLAGS) $(defines) -I$(top_srcdir)
libmate_media_profiles_la_LIBADD = $(GMP_LIBS)
libmate_media_profiles_la_LDFLAGS = -no-undefined

schemadir = $(MATECONF_SCHEMA_FILE_DIR)
schema_DATA = mate-audio-profiles.schemas

install-schemas:
	MATECONF_CONFIG_SOURCE=$(MATECONF_SCHEMA_CONFIG_SOURCE) $(MATECONFTOOL) --makefile-install-rule $(top_builddir)/profiles/$(schema_DATA) || true

if MATECONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-media-profiles.pc
EXTRA_DIST = $(ui_DATA) $(schema_DATA) $(pkgconfig_DATA)

CLEANFILES = $(schema_DATA)

# "." is dirty hack to first build our .la in this dir before moving to glade
SUBDIRS =  . help

if HAVE_GLADEUI
SUBDIRS += glade
endif

-include $(top_srcdir)/git.mk
