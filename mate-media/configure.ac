AC_PREREQ(2.60)

AC_INIT([mate-media],
        [2.32.0],
        [http://bugzilla.mate.org/enter_bug.cgi?product=mate-media])

AC_CONFIG_AUX_DIR([build-aux])

AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-bzip2 -Wall -Wno-portability tar-pax foreign])

AS_VERSION

AM_MAINTAINER_MODE

if type -p stow > /dev/null && test -d /usr/local/stow ; then
   AC_MSG_NOTICE([*** Found /usr/local/stow: default install prefix set to /usr/local/stow/${PACKAGE}-${VERSION} ***])
   ac_default_prefix="/usr/local/stow/${PACKAGE}-${VERSION}"
fi

AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

IT_PROG_INTLTOOL([0.35.0])

AC_PROG_CC
AC_ISC_POSIX

# Checks for programs.
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h malloc.h strings.h sys/ioctl.h unistd.h])
AC_CHECK_HEADERS([sys/sockio.h sys/param.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MMAP

# CFLAGS
MATE_COMPILE_WARNINGS

# AS_NANO(GM_RELEASE="yes", GM_RELEASE="no")
AS_COMPILER_FLAG(-Wall, GM_ERROR_CFLAGS="-Wall")

if test "x$GM_RELEASE" = "xno"
then
  AS_COMPILER_FLAG(-Werror, GM_ERROR_CFLAGS="$GM_ERROR_CFLAGS -Werror")
fi
AC_MSG_NOTICE([Using $GM_ERROR_CFLAGS as error checking CFLAGS])

GLIB_REQUIRED_VERSION=2.18.2
GTK_REQUIRED_VERSION=2.18.0
MATECONF_REQUIRED_VERSION=2.6.1
PA_REQUIRED_VERSION=0.9.16
CANBERRA_REQUIRED_VERSION=0.13

dnl=======================================================================
dnl Check for libcanberra
dnl=======================================================================

PKG_CHECK_MODULES(SOUND_THEME,
        gobject-2.0 >= $GLIB_REQUIRED_VERSION
        gtk+-2.0 >= $GTK_REQUIRED_VERSION
        gio-2.0
        mateconf-2.0 >= $MATECONF_REQUIRED_VERSION
        libcanberra-gtk >= $CANBERRA_REQUIRED_VERSION
        libxml-2.0,
        have_soundtheme=yes,
        have_soundtheme=no)

if test "x$have_soundtheme" = "xyes" ; then
  AC_DEFINE(HAVE_SOUND_THEME, [], [Define if we can build sound theme])
fi
AM_CONDITIONAL(HAVE_SOUND_THEME, test x$have_soundtheme = xyes)
AC_SUBST(HAVE_SOUND_THEME)
AC_SUBST(SOUNDTHEME_CFLAGS)
AC_SUBST(SOUNDTHEME_LIBS)

dnl=======================================================================
dnl Check for the new volume control modules
dnl=======================================================================

dnl The new mate-volume-control needs a newer GTK+
GTK_REQUIRED_VERSION=2.15.1

AC_ARG_ENABLE([pulseaudio],
              AS_HELP_STRING([--enable-pulseaudio],
                             [Enable PulseAudio support @<:@default=auto@:>@]),
              enable_pulseaudio=$enableval, enable_pulseaudio=auto)

if test "x$enable_pulseaudio" != "xno"; then

  PKG_CHECK_MODULES(PULSEAUDIO,
          libpulse >= $PA_REQUIRED_VERSION libpulse-mainloop-glib >= $PA_REQUIRED_VERSION,
          have_pulseaudio=yes,
          have_pulseaudio=no)

  if test "x$enable_pulseaudio" = "xyes" -a "x$have_pulseaudio" = "xno"; then
    AC_MSG_ERROR([PulseAudio support explicitly requested but dependencies not found])
  fi

  if test "x$have_pulseaudio" = "xyes" ; then
    PKG_CHECK_MODULES(VOLUME_CONTROL,
            gobject-2.0 >= $GLIB_REQUIRED_VERSION
            gtk+-2.0 >= $GTK_REQUIRED_VERSION
            gio-2.0
            mateconf-2.0 >= $MATECONF_REQUIRED_VERSION
            libcanberra-gtk >= $CANBERRA_REQUIRED_VERSION
            unique-1.0
            libxml-2.0
    )
    AC_SUBST(VOLUME_CONTROL_CFLAGS)
    AC_SUBST(VOLUME_CONTROL_LIBS)
    AC_DEFINE(HAVE_PULSEAUDIO, [], [Define if we have pulseaudio])
  fi
else
  have_pulseaudio=no
fi
AM_CONDITIONAL(HAVE_PULSEAUDIO, test x$have_pulseaudio = xyes)
AC_SUBST(HAVE_PULSEAUDIO)
AC_SUBST(PULSEAUDIO_CFLAGS)
AC_SUBST(PULSEAUDIO_LIBS)

dnl=======================================================================
dnl profiles
dnl=======================================================================

have_profiles=no

AC_ARG_ENABLE([profiles],
  AS_HELP_STRING([--enable-profiles], [enable mate media profiles]),
  ,enable_profiles=yes)

if test "x$enable_profiles" = "xyes"; then
  PKG_CHECK_MODULES(GMP, [
    gtk+-2.0 >= $GTK_REQUIRED_VERSION
    gstreamer-0.10
    mateconf-2.0
    gio-2.0],
    have_profiles=yes)
fi
AC_SUBST(GMP_CFLAGS)
AC_SUBST(GMP_LIBS)
AM_CONDITIONAL(HAVE_GMP, [test x$have_profiles = xyes])

dnl Profiles glade ui: needed to edit .ui files with glade3
have_gladeui=no

PKG_CHECK_MODULES(GLADEUI, [gladeui-1.0], have_gladeui=yes, have_gladeui=no)
AC_SUBST(GLADEUI_CFLAGS)
AC_SUBST(GLADEUI_LIBS)
AM_CONDITIONAL(HAVE_GLADEUI, [test x$have_gladeui = xyes])

GLADEUI_CATALOG_DIR="`$PKG_CONFIG --variable=catalogdir gladeui-1.0`"
AC_SUBST(GLADEUI_CATALOG_DIR)

GLADEUI_MODULE_DIR="`$PKG_CONFIG --variable=moduledir gladeui-1.0`"
AC_SUBST(GLADEUI_MODULE_DIR)

dnl=======================================================================
dnl Check for GStreamer and GSR deps
dnl=======================================================================

GST_REQ=0.10.23
GST_PLUGINS_BASE_REQ=0.10.23
GST_MAJORMINOR=0.10

dnl make available to Makefile.am
AC_SUBST(GST_MAJORMINOR)
AC_DEFINE_UNQUOTED(GST_MAJORMINOR_QUOTED, "$GST_MAJORMINOR",
  [major/minor of GStreamer as a string constant])

have_gst=no

PKG_CHECK_MODULES(GSR, [
  gstreamer-$GST_MAJORMINOR >= $GST_REQ
  gstreamer-plugins-base-$GST_MAJORMINOR >= $GST_REQ
  gtk+-2.0 >= 2.6.0
  gio-2.0
  mateconf-2.0],
  have_gst=yes,
  [
    AC_MSG_RESULT(no)
    AC_MSG_WARN($GSR_PKG_ERRORS)
    PROGRAMS_GRECORD_ERROR="$GSR_PKG_ERRORS"
  ])

have_grecord=no
grecord_reason=""

AC_ARG_ENABLE([grecord],
  AS_HELP_STRING([--enable-grecord], [enable mate sound recorder]),
  ,enable_grecord=yes)

if test "x$have_profiles" = "xno" ; then
  AC_MSG_WARN([Grecord will be disabled because of disabled profiles])
  enable_grecord=no
  have_grecord=no
  grecord_reason=", mate media profiles is disabled"
fi

if test "x$have_gst" = "xyes" && test "x$enable_grecord" = "xyes"; then
  have_grecord=yes
else
  if test "x$have_gst" = "xno"; then
    AC_MSG_WARN([You need GStreamer installed for the mixer and recorder to work])
    sleep 5 2>/dev/null
  fi
fi

AC_SUBST(GSR_CFLAGS)
AC_SUBST(GSR_LIBS)
AM_CONDITIONAL(HAVE_GSTREAMER, [test x$have_gst = xyes])
AM_CONDITIONAL(HAVE_GRECORD, [test x$have_grecord = xyes])

dnl uninstalled gsr ui dir
AC_DEFINE_UNQUOTED(GSR_UIDIR_UNINSTALLED, "`pwd`/grecord/src",
  [path to uninstalled mate-sound-recorder UI dir])

AC_PATH_PROG(MATECONFTOOL, mateconftool-2)
AM_MATECONF_SOURCE_2

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl=======================================================================
dnl i18n
dnl=======================================================================

GETTEXT_PACKAGE=mate-media-2.0
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the Gettext package name])
AM_GLIB_GNU_GETTEXT

MATE_DOC_INIT

AM_PROG_LIBTOOL
AM_PROG_CC_C_O

dnl=======================================================================
dnl gstreamer-properties
dnl=======================================================================

have_gstprops=no

AC_ARG_ENABLE([gstprops],
  AS_HELP_STRING([--enable-gstprops], [enable gstreamer properties]),
  ,enable_gstprops=yes)

if test "x$have_gst" = "xyes" && test "x$enable_gstprops" = "xyes";
then
  PKG_CHECK_MODULES(GSTPROPS, [
    glib-2.0
    gtk+-2.0 >= $GTK_REQUIRED_VERSION
    mateconf-2.0
    gstreamer-$GST_MAJORMINOR
    gstreamer-plugins-base-$GST_MAJORMINOR >= $GST_PLUGINS_BASE_REQ
    libxml-2.0],
    have_gstprops=yes)

  GSTPROPS_LIBS="$GSTPROPS_LIBS -lgstinterfaces-$GST_MAJORMINOR"

  MATECC_DESKTOP_DIR=${datadir}/control-center-2.0/capplets
  AS_AC_EXPAND(MATECC_DESKTOP_DIR, $MATECC_DESKTOP_DIR)
  AC_DEFINE_UNQUOTED(MATECC_DESKTOP_DIR, "$MATECC_DESKTOP_DIR",
                     [Install path for Mate control-center integration])
  AC_SUBST(MATECC_DESKTOP_DIR)
  AC_SUBST(PROGRAMS_GSTPROPS)
fi

AC_SUBST(GSTPROPS_CFLAGS)
AC_SUBST(GSTPROPS_LIBS)
AM_CONDITIONAL(HAVE_GSTPROPS, [test x$have_gstprops = xyes])

dnl=======================================================================
dnl gst-mixer
dnl=======================================================================

have_gstmixer=no

AC_ARG_ENABLE([gstmix],
  AS_HELP_STRING([--enable-gstmix], [enable gstreamer mixer]),
  ,enable_gstmix=auto)

if test "x$enable_gstmix" = "xauto" && test "x$have_pulseaudio" = "xno"; then
  enable_gstmix=yes
fi

if test "x$have_gst" = "xyes" && test "x$enable_gstmix" = "xyes";
then
  PKG_CHECK_MODULES(GSTMIXER, [
    gtk+-2.0
    mateconf-2.0
    gstreamer-$GST_MAJORMINOR
    gstreamer-plugins-base-$GST_MAJORMINOR
  ],
  [
    OLD_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $GSTMIXER_CFLAGS"

    AC_CHECK_HEADER(gst/audio/mixerutils.h,
      [AC_CHECK_HEADER(gst/interfaces/mixer.h,
        [AC_CHECK_HEADER(gst/interfaces/propertyprobe.h, have_gstmixer=yes)])])

    CPPFLAGS=$OLD_CPPFLAGS
    GSTMIXER_LIBS="$GSTMIXER_LIBS -lgstinterfaces-$GST_MAJORMINOR -lgstaudio-$GST_MAJORMINOR"
  ])
fi

AC_SUBST(GSTMIXER_CFLAGS)
AC_SUBST(GSTMIXER_LIBS)
AM_CONDITIONAL(HAVE_GSTMIXER, [test x$have_gstmixer = xyes])


dnl ---------------------------------------------------------------------------
dnl - Finish
dnl ---------------------------------------------------------------------------

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

MATE_MAINTAINER_MODE_DEFINES
MATE_COMPILE_WARNINGS([maximum])
MATE_CXX_WARNINGS

# Turn on the additional warnings last, so warnings don't affect other tests.
AC_ARG_ENABLE(more-warnings,
        [AC_HELP_STRING([--enable-more-warnings],
        [Maximum compiler warnings])],
        set_more_warnings="$enableval",[
                if test -d $srcdir/.git; then
                        set_more_warnings=yes
                else
                        set_more_warnings=no
                fi
        ])
AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="\
        -Wall \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wsign-compare \
        $CFLAGS"

        for option in -Wno-unused-parameter -Wno-strict-aliasing -Wno-sign-compare; do
                SAVE_CFLAGS="$CFLAGS"
                CFLAGS="$CFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_TRY_COMPILE([], [],
                        has_option=yes,
                        has_option=no,)
                if test $has_option = no; then
                        CFLAGS="$SAVE_CFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
else
        AC_MSG_RESULT(no)
fi


AC_SUBST(CFLAGS)
AC_SUBST(CPPFLAGS)
AC_SUBST(LIBS)
AC_SUBST(LDFLAGS)

AC_CONFIG_FILES([
Makefile
po/Makefile.in
sound-theme/Makefile
sound-theme/sounds/Makefile
mate-volume-control/Makefile
mate-volume-control/data/Makefile
mate-volume-control/data/mate-volume-control.desktop.in
mate-volume-control/data/icons/Makefile
mate-volume-control/data/icons/16x16/Makefile
mate-volume-control/data/icons/16x16/apps/Makefile
mate-volume-control/data/icons/16x16/status/Makefile
mate-volume-control/data/icons/22x22/Makefile
mate-volume-control/data/icons/22x22/apps/Makefile
mate-volume-control/data/icons/22x22/status/Makefile
mate-volume-control/data/icons/24x24/Makefile
mate-volume-control/data/icons/24x24/apps/Makefile
mate-volume-control/data/icons/24x24/status/Makefile
mate-volume-control/data/icons/32x32/Makefile
mate-volume-control/data/icons/32x32/apps/Makefile
mate-volume-control/data/icons/32x32/status/Makefile
mate-volume-control/data/icons/48x48/Makefile
mate-volume-control/data/icons/48x48/apps/Makefile
mate-volume-control/data/icons/scalable/Makefile
mate-volume-control/data/icons/scalable/apps/Makefile
mate-volume-control/data/icons/scalable/devices/Makefile
mate-volume-control/data/icons/scalable/status/Makefile
mate-volume-control/src/Makefile
grecord/Makefile
grecord/mate-sound-recorder.schemas.in
grecord/src/Makefile
grecord/doc/Makefile
grecord/icons/Makefile
grecord/icons/16x16/Makefile
grecord/icons/22x22/Makefile
grecord/icons/24x24/Makefile
grecord/icons/32x32/Makefile
grecord/icons/48x48/Makefile
grecord/icons/scalable/Makefile
gstreamer-properties/Makefile
gstreamer-properties/help/Makefile
gst-mixer/Makefile
gst-mixer/icons/Makefile
gst-mixer/icons/16x16/Makefile
gst-mixer/icons/16x16/devices/Makefile
gst-mixer/icons/16x16/status/Makefile
gst-mixer/src/Makefile
gst-mixer/pixmaps/Makefile
gst-mixer/doc/Makefile
profiles/Makefile
profiles/mate-audio-profiles.schemas.in
profiles/mate-media-profiles.pc
profiles/glade/Makefile
profiles/help/Makefile
grecord/mate-sound-recorder.desktop.in
gst-mixer/mate-volume-control.desktop.in
gstreamer-properties/gstreamer-properties.desktop.in
])

AC_OUTPUT

echo "

                    ${PACKAGE} ${VERSION}
                    ============


        Prefix:                 ${prefix}
        Source code location:   ${srcdir}
        Compiler:               ${CC}
        CFLAGS:                 ${CFLAGS}

        Volume Control          $have_pulseaudio
        GStreamer properties:   $have_gstprops
        GStreamer profiles:     $have_profiles (GladeUI: $have_gladeui)
        Sound recorder:         $have_grecord$grecord_reason

  *Deprecated*:

        GStreamer mixer:        $have_gstmixer

"

