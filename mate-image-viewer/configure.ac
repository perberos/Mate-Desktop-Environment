AC_PREREQ(2.59)

m4_define(eog_major_version,  2)
m4_define(eog_minor_version, 32)
m4_define(eog_micro_version, 0)
m4_define(eog_version, eog_major_version.eog_minor_version.eog_micro_version)

AC_INIT([eog], eog_version, [http://bugzilla.mate.org/enter_bug.cgi?product=eog], [eog])
AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-bzip2])

# Support silencing the build output if supported (automake-1.11+)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_DEFINE(EOG_MAJOR_VERSION, eog_major_version, [EOG major version])
AC_SUBST(EOG_MAJOR_VERSION,  eog_major_version)
AC_DEFINE(EOG_MINOR_VERSION, eog_minor_version, [EOG minor version])
AC_SUBST(EOG_MINOR_VERSION,  eog_minor_version)
AC_DEFINE(EOG_MICRO_VERSION, eog_micro_version, [EOG micro version])
AC_SUBST(EOG_MICRO_VERSION,  eog_micro_version)

EOG_API_VERSION=2.20
AC_SUBST(EOG_API_VERSION)

AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE

MATE_COMMON_INIT
MATE_MAINTAINER_MODE_DEFINES
MATE_COMPILE_WARNINGS
MATE_DEBUG_CHECK

# Disable building static archives by default
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])
AC_PATH_PROG([GLIB_MKENUMS],[glib-mkenums])

GTK_DOC_CHECK(1.9)

AC_CHECK_FUNCS(strptime)

# ****************************************************************
# Support for nl_langinfo (_NL_MEASUREMENT_MEASUREMENT) (optional)
# ****************************************************************
AC_MSG_CHECKING([for _NL_MEASUREMENT_MEASUREMENT])
AC_TRY_LINK([#include <langinfo.h>], [
char c;
c = *((unsigned char *)  nl_langinfo(_NL_MEASUREMENT_MEASUREMENT));
], have_langinfo_measurement=yes, have_langinfo_measurement=no)
AC_MSG_RESULT($have_langinfo_measurement)
if test "$have_langinfo_measurement" = "yes"; then
  AC_DEFINE([HAVE__NL_MEASUREMENT_MEASUREMENT], [1],
            [Define if _NL_MEASUREMENT_MEASUREMENT is available])
fi

# ***********
# Translation
# ***********

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.40.0])

GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Gettext package.])

AC_SUBST(CFLAGS)
AC_SUBST(LDFLAGS)

# ***************
# mate-doc-utils
# ***************

PKG_PROG_PKG_CONFIG
MATE_DOC_INIT

# **********
# MATE Libs
# **********

GTK_REQUIRED=2.18.0
GLIB_REQUIRED=2.25.9
MATE_DESKTOP_REQUIRED=2.25.1
GDKPIXBUF_REQUIRED=2.4.0
MATECONF_REQUIRED=2.5.90
GTK_PRINT_REQUIRED=2.10.0
MATE_ICON_THEME_REQUIRED=2.19.1
SHARED_MIME_INFO_REQUIRED=0.20
EXEMPI_REQUIRED=1.99.5

EOG_MODULES="gtk+-2.0 >= $GTK_REQUIRED \
             glib-2.0 >= $GLIB_REQUIRED \
             gio-2.0 >= $GLIB_REQUIRED \
             gthread-2.0 >= $GLIB_REQUIRED \
             mate-desktop-2.0 >= $MATE_DESKTOP_REQUIRED \
             gdk-pixbuf-2.0 >= $GDKPIXBUF_REQUIRED \
             mateconf-2.0 >= $MATECONF_REQUIRED \
             gtk+-unix-print-2.0 >= $GTK_PRINT_REQUIRED \
	     mate-icon-theme >= $MATE_ICON_THEME_REQUIRED \
	     shared-mime-info >= $SHARED_MIME_INFO_REQUIRED"

# ***************
# ZLIB (required)
# ***************

have_zlib=yes
AC_CHECK_HEADERS([zlib.h],
	[AC_CHECK_LIB([z], [inflate],
		[AC_CHECK_LIB([z], [crc32], [], [have_zlib=no])],
		[have_zlib=no])],
	[have_zlib=no])

if test x$have_zlib = xno; then
	AC_MSG_ERROR([No sufficient zlib library found on your system.])
fi

# ***************
# EXIF (optional)
# ***************

LIBEXIF_REQUIRED=0.6.14

AC_ARG_WITH([libexif], AC_HELP_STRING([--without-libexif], [disable EXIF support]))
have_exif=no
if test x$with_libexif != xno; then
    PKG_CHECK_MODULES(EXIF, libexif >= $LIBEXIF_REQUIRED, have_exif=yes, have_exif=no)
    if test "x$have_exif" = "xyes"; then
        AC_CHECK_HEADER([libexif/exif-data.h],[],[have_exif=no])
    fi
fi
if test "x$have_exif" = "xyes"; then
  AC_DEFINE(HAVE_EXIF, 1, [EXIF Support.])
  EOG_MODULES="$EOG_MODULES libexif >= $LIBEXIF_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXIF], [test "x$have_exif" = "xyes"])

# *********************
# Little CMS (optional)
# *********************

AC_ARG_WITH([cms], AC_HELP_STRING([--without-cms], [disable colour management support]))
have_lcms=no
if test x$with_cms != xno; then
    PKG_CHECK_MODULES(LCMS, lcms, have_lcms=yes, have_lcms=no)
fi
if test "x$have_lcms" = "xyes"; then
  AC_DEFINE(HAVE_LCMS, 1, [Little CMS Support.])
  EOG_MODULES="$EOG_MODULES lcms"
fi

AM_CONDITIONAL([HAVE_LCMS], [test "x$have_lcms" = "xyes"])

# *********************
# Exempi (optional)
# *********************
AC_ARG_WITH(xmp, AC_HELP_STRING([--without-xmp], [disable special XMP support]))
have_exempi=no
if test x$with_xmp != xno; then
    PKG_CHECK_MODULES(EXEMPI, exempi-2.0 >= $EXEMPI_REQUIRED, have_exempi=yes, have_exempi=no)
fi
if test "x$have_exempi" = "xyes"; then
  AC_DEFINE(HAVE_EXEMPI,1, [XMP support.])
  EOG_MODULES="$EOG_MODULES exempi-2.0 >= $EXEMPI_REQUIRED"
fi

AM_CONDITIONAL([HAVE_EXEMPI], [test "x$have_exempi" = "xyes"])

# Build EXIF-related documentation if we really have support for it
if test "x$have_exif" = "xyes" || test "x$have_exempi" = "xyes"; then
  EOG_DOC_EXIF_START=""
  EOG_DOC_EXIF_STOP=""
else
  EOG_DOC_EXIF_START="<!--"
  EOG_DOC_EXIF_STOP="-->"
fi

AC_SUBST([EOG_DOC_EXIF_START])
AC_SUBST([EOG_DOC_EXIF_STOP])

# ********************
# Jpeg (semi-optional)
# ********************

AC_ARG_WITH(libjpeg, AC_HELP_STRING([--without-libjpeg], [disable special JPEG support]))
  have_jpeg=no
  have_libjpeg_80=no
  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
    AC_CHECK_LIB(jpeg, jpeg_destroy_decompress,
      have_jpeg=yes,
      have_jpeg=no
      AC_MSG_WARN(*** JPEG loader will not be built (JPEG library not found) ***))
    if test "$have_jpeg" = yes; then
      AC_MSG_CHECKING([for jpeglib.h])
      AC_TRY_CPP(
[#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>],
        have_jpeg=yes,
        have_jpeg=no)
      AC_MSG_RESULT($have_jpeg)
      if test "$have_jpeg" = yes; then
        LIBJPEG='-ljpeg'
	AC_DEFINE(HAVE_JPEG, 1, [Jpeg Support.])
        AC_DEFINE(HAVE_LIBJPEG, 1, [libjpeg is Present.])
        AC_CHECK_LIB(jpeg, jpeg_simple_progression,
          AC_DEFINE(HAVE_PROGRESSIVE_JPEG, 1, [Jpeg Progressive Saving Support.]),
          AC_MSG_WARN(JPEG library does not support progressive saving.))
	AC_MSG_CHECKING([[libjpeg version is 8 or greater]])
	AC_PREPROC_IFELSE([ AC_LANG_SOURCE(
			  [[
#include <stdio.h>
#include <jpeglib.h>
#if JPEG_LIB_VERSION < 80
#error "wrong version"
#endif
int main(int c, char**v) { return 0; }
			  ]]) ],
			  [AC_MSG_RESULT(yes)
			   have_libjpeg_80=yes],
		          [AC_MSG_RESULT(no)
		           have_libjpeg_80=no])

      else
          AC_MSG_WARN(*** JPEG loader will not be built (JPEG header file not found) ***)
      fi
    fi
  fi

  if test x$with_libjpeg != xno && test -z "$LIBJPEG"; then
     AC_MSG_ERROR([
*** Checks for JPEG loader failed. You can build without it by passing
*** --without-libjpeg to configure but some programs using GTK+ may
*** not work properly])
  fi
AC_SUBST(LIBJPEG)
AM_CONDITIONAL(ENABLE_JPEG, test x$have_jpeg = xyes)
AM_CONDITIONAL(HAVE_LIBJPEG_80, test "x$have_libjpeg_80" = xyes)

# ****************
# D-Bus activation
# ****************

DBUS_GLIB_REQUIRED=0.71

AC_ARG_WITH([dbus], AC_HELP_STRING([--without-dbus], [disable dbus based activation]))
have_dbus=no
if test x$with_dbus != xno; then
    PKG_CHECK_MODULES([DBUS], [dbus-glib-1 >= $DBUS_GLIB_REQUIRED], have_dbus=yes, have_dbus=no)
fi
if test x$have_dbus = xyes; then
   AC_PATH_PROG([DBUS_BINDING_TOOL], [dbus-binding-tool], [no])

   if test x$DBUS_BINDING_TOOL = "xno" ; then
       AC_MSG_ERROR([dbus-binding-tool executable not found in your path - should be installed with dbus glib bindings])
   fi

    AC_DEFINE(HAVE_DBUS, 1, [DBUS based activation.])
    EOG_MODULES="$EOG_MODULES dbus-glib-1 >= $DBUS_GLIB_REQUIRED"
fi

AM_CONDITIONAL([HAVE_DBUS], [test "x$have_dbus" = "xyes"])

# ************************************
# libXML2 (required for toolbareditor)
# ************************************

LIBXML2_REQUIRED=2.0
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= $LIBXML2_REQUIRED])

# ***************
# RSVG (optional for scaling svg image)
# ***************

LIBRSVG_REQUIRED=2.26.0

AC_ARG_WITH([librsvg], AC_HELP_STRING([--without-librsvg], [disable RSVG support]))
have_rsvg=no
if test x$with_librsvg != xno; then
    PKG_CHECK_MODULES(RSVG, librsvg-2.0 >= $LIBRSVG_REQUIRED, have_rsvg=yes, have_rsvg=no)
fi
if test "x$have_rsvg" = "xyes"; then
  AC_DEFINE(HAVE_RSVG, 1, [RSVG Support.])
  EOG_MODULES="$EOG_MODULES librsvg-2.0 >= $LIBRSVG_REQUIRED"
fi

AM_CONDITIONAL([HAVE_RSVG], [test "x$have_rsvg" = "xyes"])

# ****************
# CFLAGS/LIBS init
# ****************

PKG_CHECK_MODULES(EOG, $EOG_MODULES)
AC_SUBST(EOG_CFLAGS)
AC_SUBST(EOG_LIBS)

# *******************************************************
# libX11 (required for TotemScrSaver and Color Profiling)
# *******************************************************

# Check GDK rendering target
gdk_target=`$PKG_CONFIG --variable=target gdk-2.0`

if test "x$have_lcms" = "xyes" || test "$gdk_target" = "x11"; then
	PKG_CHECK_MODULES(X11, [x11], have_x11=yes, have_x11=no)

	if test "x$have_x11" != "xyes"; then
		# Fallback for X11 releases without .pc-files
		AC_PATH_XTRA
		if test x$no_x = xyes ; then
			AC_MSG_ERROR([X development libraries (libX11) not found])
		else
			X11_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
			AC_SUBST(X11_LIBS)
			have_x11=yes
		fi
	fi
fi

# *****
# MateConf
# *****

AM_MATECONF_SOURCE_2

AC_PATH_PROG([MATECONFTOOL], [mateconftool-2], [no])

if test "$MATECONFTOOL" = "no"; then
        AC_MSG_ERROR([mateconftool-2 not found])
fi

# ************
# Sun Keyboard
# ************

AC_CHECK_HEADERS(X11/Sunkeysym.h, AC_DEFINE(HAVE_SUNKEYSYM_H, 1, [Sun Keyboard Support]))

# **************
# Python Support
# **************

AC_MSG_CHECKING([whether Python support is requested])

AC_ARG_ENABLE([python],
	AS_HELP_STRING([--enable-python],[Enable python support]),
	[enable_python=$enableval have_python=$enableval],
	[enable_python=autodetect have_python=yes])

AC_MSG_RESULT([$enable_python])

if test "x$have_python" != "xno"; then
	AM_PATH_PYTHON([2.3],[],[no])
	if test "x$PYTHON" = "x:"; then
		have_python=no
	fi
fi

if test "x$have_python" != "xno"; then
	PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
	PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
	PYTHON_LIBS="-lpython$PYTHON_VERSION"
	PYTHON_LIB_LOC="-L$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config"
	PYTHON_CFLAGS="-I$PY_PREFIX/include/python$PYTHON_VERSION"
	PYTHON_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PYTHON_VERSION/config/Makefile"
	PYTHON_BASEMODLIBS=`sed -n -e 's/^BASEMODLIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_OTHER_LIBS=`sed -n -e 's/^LIBS=\(.*\)/\1/p' $PYTHON_MAKEFILE`
	PYTHON_EXTRA_LIBS="$PYTHON_BASEMODLIBS $PYTHON_OTHER_LIBS"
	AC_SUBST([PYTHON_LIBS])
	AC_SUBST([PYTHON_LIB_LOC])
	AC_SUBST([PYTHON_CFLAGS])
	AC_SUBST([PYTHON_EXTRA_LIBS])
fi

if test "x$have_python" != "xyes"; then
	if test "x$enable_python" = "xyes"; then
		AC_MSG_ERROR([Python not found])
	elif test "x$enable_python" = "xautodetect"; then
		enable_python=no
		AC_MSG_WARN([Python not found, disabling python support])
	fi
fi

if test "x$have_python" != "xno"; then
	PYGOBJECT_REQUIRED=2.15.1
	PYGTK_REQUIRED=2.13.0

	PKG_CHECK_MODULES([PYGTK], [
		pygobject-2.0 >= $PYGOBJECT_REQUIRED
		pygtk-2.0 >= $PYGTK_REQUIRED],
 		[],
 		[
 		have_python=no
 		if test "x$enable_python" = "xyes"; then
 			AC_MSG_ERROR([$PYGTK_PKG_ERRORS])
 		elif test "x$enable_python" = "xautodetect"; then
 			enable_python=no
 			AC_MSG_WARN([$PYGTK_PKG_ERRORS])
 			AC_MSG_WARN([Disabling python support])
 		fi
 		])

	AC_SUBST([PYGTK_CFLAGS])
	AC_SUBST([PYGTK_LIBS])
fi

if test "x$have_python" != "xno"; then
	AC_MSG_CHECKING([for pygobject defs])
	PYGOBJECT_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygobject-2.0`
	AC_MSG_RESULT([$PYGOBJECT_DEFSDIR])

	AC_MSG_CHECKING([for pygtk defs])
	PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
	AC_MSG_RESULT([$PYGTK_DEFSDIR])

	AC_MSG_CHECKING([for pygobject codegen])
	PYGOBJECT_CODEGEN="$PYTHON `$PKG_CONFIG --variable=codegendir pygobject-2.0`/codegen.py"
	AC_MSG_RESULT([$PYGOBJECT_CODEGEN])

	AC_MSG_CHECKING([for pygobject h2def])
	PYGOBJECT_H2DEF="$PYTHON `$PKG_CONFIG --variable=codegendir pygobject-2.0`/h2def.py"
	AC_MSG_RESULT([$PYGOBJECT_H2DEF])

	AC_SUBST([PYGOBJECT_DEFSDIR])
	AC_SUBST([PYGTK_DEFSDIR])
	AC_SUBST([PYGOBJECT_CODEGEN])
	AC_SUBST([PYGOBJECT_H2DEF])

	dnl Check for -fno-strict-aliasing
	FLAGS="-fno-strict-aliasing"
	save_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $FLAGS"
	AC_MSG_CHECKING([whether [$]CC understands $FLAGS])
	AC_TRY_COMPILE([], [], [compiler_has_option=yes], [compiler_has_option=no])
	CFLAGS="$save_CFLAGS"
	AC_MSG_RESULT($compiler_has_option)
	if test $compiler_has_option = yes; then
		NO_STRICT_ALIASING_CFLAGS="$FLAGS"
	fi
	AC_SUBST([NO_STRICT_ALIASING_CFLAGS])
fi

if test "x$have_python" != "xno" -a "x$enable_python" != "xno"; then
	enable_python=yes
	AC_DEFINE([ENABLE_PYTHON],[1],[Define to compile with python support])
fi

AM_CONDITIONAL([ENABLE_PYTHON],[test "x$enable_python" = "xyes"])

AC_CONFIG_FILES([
Makefile
src/Makefile
jpegutils/Makefile
cut-n-paste/Makefile
cut-n-paste/toolbar-editor/Makefile
cut-n-paste/totem-screensaver/Makefile
help/Makefile
po/Makefile.in
data/Makefile
data/eog.pc
data/eog.desktop.in
data/pixmaps/Makefile
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/32x32/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
doc/Makefile
doc/reference/Makefile
doc/reference/version.xml
doc/reference/eog-docs.sgml
plugins/Makefile
bindings/Makefile
bindings/python/Makefile
])

AC_OUTPUT

echo "
Configure summary:

	Source code location .......:  ${srcdir}
	Compiler ...................:  ${CC}
	Extra Compiler Warnings ....:  ${WARN_CFLAGS}

	Python support .............:  ${have_python}
	EXIF support ...............:  ${have_exif}
	XMP support ................:  ${have_exempi}
	JPEG support ...............:  ${have_jpeg}
	Colour management support ..:  ${have_lcms}
	D-Bus activation............:  ${have_dbus}
"
