AC_PREREQ(2.62)

AC_INIT([mate-menus], [2011.09.12], [https://github.com/perberos/mate-desktop-environment])
AC_CONFIG_SRCDIR(libmenu/gmenu-tree.h)

AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])
AC_CONFIG_MACRO_DIR([m4])
AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE
MATE_MAINTAINER_MODE_DEFINES

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LIB_MENU_LT_VERSION=6:9:4
AC_SUBST(LIB_MENU_LT_VERSION)

GETTEXT_PACKAGE=mate-menus
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Name of default gettext domain])
AC_SUBST(GETTEXT_PACKAGE)

AM_GLIB_GNU_GETTEXT

IT_PROG_INTLTOOL([0.40.0])

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_ARG_PROGRAM
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "$ACLOCAL_FLAGS")

PKG_CHECK_MODULES(GLIB, glib-2.0 >= 2.6.0 gio-2.0 >= 2.15.2)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

MATE_COMPILE_WARNINGS(yes)

AC_ARG_ENABLE(deprecation_flags,
              [AC_HELP_STRING([--enable-deprecation-flags],
                              [use *_DISABLE_DEPRECATED flags @<:@default=no@:>@])],,
              [enable_deprecation_flags=no])

if test "x$enable_deprecation_flags" = "xyes"; then
   DISABLE_DEPRECATED_CFLAGS=$DISABLE_DEPRECATED
   AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
fi

dnl --enable-debug=(yes|minimum|no)
AC_ARG_ENABLE(debug,
              [AC_HELP_STRING([--enable-debug],
                              [turn on debugging @<:@default=minimum@:>@])],,
              [enable_debug=minimum])
if test "$enable_debug" = "yes"; then
  DEBUG_CFLAGS="-DG_ENABLE_DEBUG"
else
  if test "x$enable_debug" = "xno"; then
    DEBUG_CFLAGS="-DG_DISABLE_ASSERT -DG_DISABLE_CHECKS"
  else
    DEBUG_CFLAGS=""
    # make sure we have the right string for the configure summary
    enable_debug="minimum"
  fi
fi
AC_SUBST(DEBUG_CFLAGS)

AC_ARG_ENABLE(python,
              [AC_HELP_STRING([--enable-python],
                              [build python bindings @<:@default=auto@:>@])],
              [enable_deprecations=$enableval],
              [enable_deprecations=auto])

# Detect if we can build Python bindings (need python and python headers)
if test "x$enable_python" = "xno" ; then
  have_python=no
else
  AC_MSG_NOTICE([Checking to see if we can build Python bindings])
  have_python=no
  AM_PATH_PYTHON(2.3)

  if test "x$PYTHON" = "x" ; then
    AC_MSG_WARN([Python not found])
  else
    AM_CHECK_PYTHON_HEADERS(have_python_headers=yes, have_python_headers=no)
    if test "x$have_python_headers" = "xyes" ; then
      have_python=yes
    fi
  fi

  if test "x$have_python" = "xno" ; then
    if test "x$enable_python" = "xyes" ; then
      AC_MSG_ERROR([Building python explicitly requested, but can't build python bindings])
    else
      AC_MSG_WARN([Couldn't find the Python headers, not building Python bindings])
    fi
  fi
fi

AM_CONDITIONAL(HAVE_PYTHON, test x$have_python = xyes)

# Because of the way Python implements polymorphism, we get the following warning:
# "warning: dereferencing type-punned pointer will break strict-aliasing rules"
# -fno-strict-aliasing (as used in Python build) switches warnings off
NO_STRICT_ALIASING_CFLAGS=""
if test "x$GCC" = "xyes" ; then
  AC_MSG_CHECKING(whether $CC accepts -fno-strict-aliasing)
  ac_save_cc="$CC"
  CC="$CC -fno-strict-aliasing"
  AC_TRY_RUN([int main() { return 0; }],
             ac_cv_no_strict_aliasing_ok=yes,
             ac_cv_no_strict_aliasing_ok=no,
             ac_cv_no_strict_aliasing_ok=no)
  CC="$ac_save_cc"
  AC_MSG_RESULT($ac_cv_no_strict_aliasing_ok)
  if test "x$ac_cv_no_strict_aliasing_ok" = "xyes" ; then
    NO_STRICT_ALIASING_CFLAGS="-fno-strict-aliasing"
  fi
fi
AC_SUBST(NO_STRICT_ALIASING_CFLAGS)

GOBJECT_INTROSPECTION_CHECK([0.6.7])

AC_OUTPUT([
Makefile
libmenu/Makefile
libmenu/libmate-menu.pc
libmenu/libmate-menu-uninstalled.pc
layout/Makefile
desktop-directories/Makefile
util/Makefile
python/Makefile
po/Makefile.in
])

dnl ---------------------------------------------------------------------------
dnl - Show summary
dnl ---------------------------------------------------------------------------

echo "
              mate-menus $VERSION
              `echo mate-menus $VERSION | sed "s/./=/g"`

        prefix:                       ${prefix}
        exec_prefix:                  ${exec_prefix}
        libdir:                       ${libdir}
        bindir:                       ${bindir}
        sbindir:                      ${sbindir}
        sysconfdir:                   ${sysconfdir}
        localstatedir:                ${localstatedir}
        datadir:                      ${datadir}
        source code location:         ${srcdir}
        compiler:                     ${CC}
        cflags:                       ${CFLAGS}
        Maintainer mode:              ${USE_MAINTAINER_MODE}
        Use *_DISABLE_DEPRECATED:     ${enable_deprecation_flags}

        Turn on debugging:            ${enable_debug}
        Build python bindings:        ${have_python}
        Build introspection support:  ${found_introspection}

"
