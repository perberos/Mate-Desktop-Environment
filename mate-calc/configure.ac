dnl Process this file with autoconf to produce a configure script.

AC_INIT(configure.ac)
AM_INIT_AUTOMAKE(gcalctool, 5.32.0)
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE
MATE_MAINTAINER_MODE_DEFINES
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

GLIB_GSETTINGS
MATE_COMPILE_WARNINGS(maximum)

dnl ###########################################################################
dnl Dependencies
dnl ###########################################################################

GTK_REQUIRED=2.18
GIO_REQUIRED=2.25.10

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.18.0
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=2.90.4
       ;;
esac

PKG_CHECK_MODULES(GCALCTOOL, [
    gtk+-$GTK_API_VERSION >= $GTK_REQUIRED
    gio-2.0 >= $GIO_REQUIRED
    libxml-2.0
    gmodule-export-2.0
])

PKG_CHECK_MODULES(GCALCCMD, [
    glib-2.0
])

AC_CHECK_LIB(m, log)

dnl ###########################################################################
dnl Determine if a usable lex is available on this system
dnl ###########################################################################

AM_PROG_LEX
if [[ "$LEX" != "flex" ]]; then
	AC_MSG_ERROR(flex is required to create the gcalctool scanners)
fi

dnl ###########################################################################
dnl Determine if a usable yacc is available on this system
dnl ###########################################################################

AC_PROG_YACC
AC_CHECK_PROG(HAVE_YACC, $YACC, yes, no)
if [[ "$HAVE_YACC" = "no" ]]; then
	AC_MSG_ERROR($YACC is not usable as yacc - consider using bison)
fi

dnl ###########################################################################
dnl Internationalization
dnl ###########################################################################

IT_PROG_INTLTOOL([0.35.0])
AC_SUBST(GETTEXT_PACKAGE, gcalctool)

dnl ###########################################################################
dnl Documentation
dnl ###########################################################################

MATE_DOC_INIT

dnl ###########################################################################
dnl Files to generate
dnl ###########################################################################

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
data/Makefile
help/Makefile
])
