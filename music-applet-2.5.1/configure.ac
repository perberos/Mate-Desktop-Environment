dnl
dnl Preamble
dnl

AC_PREREQ(2.53)
AC_INIT([music-applet], [2.5.1], [paul@kuliniewicz.org])
AM_INIT_AUTOMAKE

AC_CONFIG_SRCDIR([src/music-applet])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

AM_MAINTAINER_MODE
AM_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS -I m4")


dnl
dnl C
dnl

AC_PROG_CC
CFLAGS="$CFLAGS -g -Wall -W -Wno-unused -Wno-missing-field-initializers"
LIBS=

AC_ARG_ENABLE([strict-checks],
	      AS_HELP_STRING([--enable-strict-checks],
			     [fail on compiler warnings]),
	      [ac_cv_enable_strict_checks=$enableval],
	      [ac_cv_enable_strict_checks=no])
AC_CACHE_CHECK([whether to enable strict checks],
	       [ac_cv_enable_strict_checks],
	       [ac_cv_enable_strict_checks=no])
if test "x$ac_cv_enable_strict_checks" = xyes; then
	CFLAGS="$CFLAGS -Werror"
fi

AC_ARG_ENABLE([deprecated],
	      AS_HELP_STRING([--disable-deprecated],
			     [disable use of deprecated APIs]),
	      [ac_cv_enable_deprecated=$enableval],
	      [ac_cv_enable_deprecated=yes])
AC_CACHE_CHECK([whether to enable deprecated APIs],
	       [ac_cv_enable_deprecated],
	       [ac_cv_enable_deprecated=yes])
if test "x$ac_cv_enable_deprecated" = xno; then
	AC_DEFINE(G_DISABLE_DEPRECATED, , [Don't use deprecated APIs from GLib])
	AC_DEFINE(GTK_DISABLE_DEPRECATED, , [Don't use deprecated APIs from GTK])
	AC_DEFINE(GDK_DISABLE_DEPRECATED, , [Don't use deprecated APIs from GDK])
	AC_DEFINE(GDK_PIXBUF_DISABLE_DEPRECATED, , [Don't use deprecated APIs from GDK-Pixbuf])
	AC_DEFINE(PANGO_DISABLE_DEPRECATED, , [Don't use deprecated APIs from Pango])
	AC_DEFINE(MATECOMPONENT_DISABLE_DEPRECATED, , [Don't use deprecated APIs from MateComponent])
	AC_DEFINE(MATE_DISABLE_DEPRECATED, , [Don't use deprecated APIs from MATE])
fi


dnl
dnl Python
dnl

AM_PATH_PYTHON(2.4)
AM_CHECK_PYTHON_HEADERS([], [AC_MSG_ERROR([could not find Python headers])])

AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
if test "x$PYGTK_CODEGEN" = xno; then
	AC_MSG_ERROR([could not find pygtk-codegen-2.0 script])
fi

PKG_CHECK_VAR(PYGTK_CODEGENDIR, [codegendir], [pygtk-2.0])
PYGTK_H2DEF="$PYGTK_CODEGENDIR/h2def.py"
AC_SUBST(PYGTK_H2DEF)

PKG_CHECK_VAR(PYGTK_DEFSDIR, [defsdir], [pygtk-2.0])

CFLAGS="$CFLAGS $PYTHON_INCLUDES"


dnl
dnl MateConf
dnl

AC_PATH_PROG(MATECONFTOOL, [mateconftool-2], [no])
if test "x$MATECONFTOOL" = xno; then
	AC_MSG_ERROR([mateconftool-2 executable not found in your path -- should be installed with MateConf])
fi
AC_SUBST(MATECONFTOOL)
AM_MATECONF_SOURCE_2


dnl
dnl i18n Support
dnl

AC_PROG_INTLTOOL([0.33])

GETTEXT_PACKAGE=music-applet
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [The gettext package])

AM_GLIB_GNU_GETTEXT


dnl
dnl Library Dependencies
dnl

PKG_CHECK_MODULES(MUSICAPPLET_DEPS, [
	gtk+-2.0
	libmatepanelapplet-2.0
	pango >= 1.6
	pygtk-2.0
	mate-python-2.0
])

CFLAGS="$CFLAGS $MUSICAPPLET_DEPS_CFLAGS"
LIBS="$LIBS $MUSICAPPLET_DEPS_LIBS"


dnl
dnl Variables
dnl

AS_AC_EXPAND(DATADIR, $datadir)
AC_SUBST(DATADIR)

AS_AC_EXPAND(LIBDIR, $libdir)
AC_SUBST(LIBDIR)

AS_AC_EXPAND(PYTHONDIR, $pythondir)
AC_SUBST(PYTHONDIR)

AS_AC_EXPAND(PKGDATADIR, $pkgdatadir)
AC_SUBST(PKGDATADIR)


dnl
dnl Non-standard Python modules that aren't technically required at configure,
dnl compile, or install time, but that may or may not be required at run time.
dnl The user will be informed of any missing modules at the end of configure
dnl time.
dnl

ma_missing_req=no
ma_missing_notify=no
ma_missing_password=no

ma_missing_amarok=no
ma_missing_audacious=no
ma_missing_banshee=no
ma_missing_exaile=no
ma_missing_mpd=no
ma_missing_muine=no
ma_missing_quodlibet=no
ma_missing_rhythmbox=no
ma_missing_vlc=no
ma_missing_xmms1=no
ma_missing_xmms2=no

AM_CHECK_PYMOD([mateconf], , , [ma_missing_req=yes])
AM_CHECK_PYMOD([mateapplet], , , [ma_missing_req=yes])
AM_CHECK_PYMOD([gobject], , , [ma_missing_req=yes])
AM_CHECK_PYMOD([gtk], , , [ma_missing_req=yes])
AM_CHECK_PYMOD([pango], , , [ma_missing_req=yes])

AM_CHECK_PYMOD([matekeyring], , , [ma_missing_password=yes])
AM_CHECK_PYMOD([pynotify], , , [ma_missing_notify=yes])

AM_CHECK_PYMOD([dbus], , , [ma_missing_audacious=yes
			    ma_missing_banshee=yes
			    ma_missing_exaile=yes
			    ma_missing_muine=yes
			    ma_missing_quodlibet=yes
			    ma_missing_rhythmbox=yes
			    ma_missing_vlc=yes])
AM_CHECK_PYMOD([dcopext], , , [ma_missing_amarok=yes])
AM_CHECK_PYMOD([kdecore], , , [ma_missing_amarok=yes])
AM_CHECK_PYMOD([mpd], , , [ma_missing_mpd=yes])
AM_CHECK_PYMOD([xmms], , , [ma_missing_xmms1=yes])
AM_CHECK_PYMOD([xmmsclient], , , [ma_missing_xmms2=yes])


dnl
dnl Output
dnl

AC_CONFIG_FILES([
	Makefile
	src/Makefile
	src/musicapplet/Makefile
	src/musicapplet/defs.py.in
	src/musicapplet/plugins/Makefile
	data/Makefile
	templates/Makefile
	po/Makefile.in
])
AC_OUTPUT


if test "x$ma_missing_req" != xno; then
	AC_MSG_WARN([The following Python modules required to run Music Applet could not be found:])
	if test "x$py_cv_mod_mateconf_" != xyes; then
		AC_MSG_WARN([    * mateconf])
	fi
	if test "x$py_cv_mod_mateapplet_" != xyes; then
		AC_MSG_WARN([    * mateapplet])
	fi
	if test "x$py_cv_mod_gobject_" != xyes; then
		AC_MSG_WARN([    * gobject])
	fi
	if test "x$py_cv_mod_gtk_" != xyes; then
		AC_MSG_WARN([    * gtk])
	fi
	if test "x$py_cv_mod_pango_" != xyes; then
		AC_MSG_WARN([    * pango])
	fi
fi

if test "x$ma_missing_notify" != xno; then
	AC_MSG_WARN([Song notification popups are disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_pynotify_" != xyes; then
		AC_MSG_WARN([    * pynotify])
	fi
fi

if test "x$ma_missing_password" != xno; then
	AC_MSG_WARN([Saving passwords is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_matekeyring_" != xyes; then
		AC_MSG_WARN([    * matekeyring])
	fi
fi

if test "x$ma_missing_amarok" != xno; then
	AC_MSG_WARN([Amarok support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dcopext" != xyes; then
		AC_MSG_WARN([    * dcopext])
	fi
	if test "x$py_cv_mod_kdecore_" != xyes; then
		AC_MSG_WARN([    * kdecore])
	fi
fi

if test "x$ma_missing_audacious" != xno; then
	AC_MSG_WARN([Audacious support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_banshee" != xno; then
	AC_MSG_WARN([Banshee support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_exaile" != xno; then
	AC_MSG_WARN([Exaile support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_mpd" != xno; then
	AC_MSG_WARN([MPD support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_mpd_" != xyes; then
		AC_MSG_WARN([    * mpd])
	fi
fi

if test "x$ma_missing_muine" != xno; then
	AC_MSG_WARN([Muine support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_quodlibet" != xno; then
	AC_MSG_WARN([Quod Libet support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_rhythmbox" != xno; then
	AC_MSG_WARN([Rhythmbox support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_vlc" != xno; then
	AC_MSG_WARN([VLC support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_dbus_" != xyes; then
		AC_MSG_WARN([    * dbus])
	fi
fi

if test "x$ma_missing_xmms1" != xno; then
	AC_MSG_WARN([XMMS1 support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_xmms_" != xyes; then
		AC_MSG_WARN([    * xmms])
	fi
fi

if test "x$ma_missing_xmms2" != xno; then
	AC_MSG_WARN([XMMS2 support is disabled because the following Python modules could not be found:])
	if test "x$py_cv_mod_xmmsclient_" != xyes; then
		AC_MSG_WARN([    * xmmsclient])
	fi
fi
