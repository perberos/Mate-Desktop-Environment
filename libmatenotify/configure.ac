dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT([libmatenotify], [2011.11.16], [https://github.com/perberos/mate-desktop-environment], [libmatenotify])
AC_PREREQ(2.50)
AC_CONFIG_SRCDIR(config.h.in)

AC_CONFIG_MACRO_DIR([m4])

dnl ################################################################
dnl # Version information
dnl ################################################################
LIBMATENOTIFY_MAJOR_VERSION=0
LIBMATENOTIFY_MINOR_VERSION=5
LIBMATENOTIFY_MICRO_VERSION=0
LIBMATENOTIFY_DEVEL_VERSION=0

LIBMATENOTIFY_VERSION=$LIBMATENOTIFY_MAJOR_VERSION.$LIBMATENOTIFY_MINOR_VERSION.$LIBMATENOTIFY_MICRO_VERSION

if test "x$LIBMATENOTIFY_DEVEL_VERSION" != "x0"; then
	LIBMATENOTIFY_VERSION=$LIBMATENOTIFY_VERSION.$LIBMATENOTIFY_DEVEL_VERSION
fi

AC_DEFINE_UNQUOTED(LIBMATENOTIFY_MAJOR_VERSION, $LIBMATENOTIFY_MAJOR_VERSION,
	[libmatenotify major version.])
AC_DEFINE_UNQUOTED(LIBMATENOTIFY_MINOR_VERSION, $LIBMATENOTIFY_MINOR_VERSION,
	[libmatenotify minor version.])
AC_DEFINE_UNQUOTED(LIBMATENOTIFY_MICRO_VERSION, $LIBMATENOTIFY_MICRO_VERSION,
	[libmatenotify micro version.])
AC_DEFINE_UNQUOTED(LIBMATENOTIFY_VERSION, "$LIBMATENOTIFY_VERSION",
	[libmatenotify version.])

dnl ################################################################
dnl # libtool versioning
dnl ################################################################
dnl #
dnl # +1 :  0 : +1   == new interface that does not break old one.
dnl # +1 :  0 :  0   == removed an interface. Breaks old apps.
dnl #  ? : +1 :  ?   == internal changes that doesn't break anything.
dnl #
dnl # CURRENT : REVISION : AGE
dnl #
LT_CURRENT=3
LT_REVISION=3
LT_AGE=2

LT_RELEASE=$LIBMATENOTIFY_MAJOR_VERSION.$LIBMATENOTIFY_MINOR_VERSION.$LIBMATENOTIFY_MICRO_VERSION

AC_SUBST(LT_RELEASE)
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
VERSION=$LIBMATENOTIFY_VERSION
PACKAGE=libmatenotify

AM_INIT_AUTOMAKE([1.10 no-dist-gzip dist-bzip2])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ################################################################
dnl # Specify a header configuration file
dnl ################################################################
AM_CONFIG_HEADER(config.h)

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
AC_PROG_CC
#AC_ARG_PROGRAM
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

AC_C_CONST
AC_TYPE_SIZE_T
AC_EXEEXT

AM_PROG_LIBTOOL

REQ_DBUS_VERSION=0.76
REQ_GTK_VERSION=2.18
REQ_GLIB_VERSION=2.6
pkg_modules="gtk+-2.0 >= $REQ_GTK_VERSION, glib-2.0 >= $REQ_GLIB_VERSION, dbus-1 >= $REQ_DBUS_VERSION, dbus-glib-1 >= $REQ_DBUS_VERSION"
AC_SUBST(pkg_modules)
PKG_CHECK_MODULES(PACKAGE, [$pkg_modules])
AC_SUBST(PACKAGE_CFLAGS)
AC_SUBST(PACKAGE_LIBS)

GLIB_GENMARSHAL=`pkg-config --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

dnl
dnl Check the D-BUS version.
dnl

AC_MSG_CHECKING([dbus version])
DBUS_VERSION=`$PKG_CONFIG --modversion dbus-1`
DBUS_MAJOR_VER=`echo $DBUS_VERSION | cut -d. -f 1`
DBUS_MINOR_VER=`echo $DBUS_VERSION | cut -d. -f 2`
DBUS_MICRO_VER=`echo $DBUS_VERSION | cut -d. -f 3`

AC_MSG_RESULT($DBUS_VERSION)
AC_DEFINE_UNQUOTED(DBUS_MAJOR_VER, $DBUS_MAJOR_VER, [D-BUS major version.])
AC_DEFINE_UNQUOTED(DBUS_MINOR_VER, $DBUS_MINOR_VER, [D-BUS minor version.])
AC_DEFINE_UNQUOTED(DBUS_MICRO_VER, $DBUS_MICRO_VER, [D-BUS micro version.])

AC_DEFINE_UNQUOTED(DBUS_API_SUBJECT_TO_CHANGE, ,DBUS API is subject to change)


dnl ################################################################
dnl # Set up gtk-doc
dnl ################################################################
GTK_DOC_CHECK([1.4])

dnl ---------------------------------------------------------------------------
dnl - DocBook Documentation
dnl ---------------------------------------------------------------------------

AC_ARG_ENABLE(docbook-docs,
	[AC_HELP_STRING([--enable-docbook-docs],
	[build documentation (requires xmlto)])],
	enable_docbook_docs=$enableval,enable_docbook_docs=no)
AC_PATH_PROG(XMLTO, xmlto, no)
AC_MSG_CHECKING([whether to build DocBook documentation])
if test x$XMLTO = xno ; then
	have_docbook=no
else
	have_docbook=yes
fi
if test x$enable_docbook_docs = xauto ; then
	 if test x$have_docbook = xno ; then
		enable_docbook_docs=no
	else
		enable_docbook_docs=yes
	fi
fi
if test x$enable_docbook_docs = xyes; then
	if test x$have_docbook = xno; then
		AC_MSG_ERROR([Building DocBook docs explicitly required, but DocBook not found])
	fi
fi
AM_CONDITIONAL(DOCBOOK_DOCS_ENABLED, test x$enable_docbook_docs = xyes)
AC_MSG_RESULT(yes)

AC_ARG_VAR([XMLTO],[Define/override the `xmlto' location.])
AC_ARG_VAR([XMLTO_FLAGS],[Define/override `xmlto' options, like `--skip-validation'.])


# Turn on the additional warnings last

AC_ARG_ENABLE(more-warnings,
              AS_HELP_STRING([--enable-more-warnings],
                             [Maximum compiler warnings]),
              set_more_warnings="$enableval",[
	      if test -d $srcdir/.git; then
	        set_more_warnings=yes
	      else
	        set_more_warnings=no
              fi])

AC_MSG_CHECKING(for more warnings)
if test "$GCC" = "yes" -a "$set_more_warnings" != "no"; then
        AC_MSG_RESULT(yes)
        CFLAGS="\
        -Wall \
        -Wchar-subscripts -Wmissing-declarations -Wmissing-prototypes \
        -Wnested-externs -Wpointer-arith \
        -Wcast-align -Wsign-compare -Wp,-D_FORTIFY_SOURCE=2 \
        $CFLAGS"

        for option in -Wno-strict-aliasing -Wno-sign-compare; do
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

dnl ################################################################
dnl # Output the Makefiles
dnl ################################################################
AC_CONFIG_FILES([
Makefile
docs/Makefile
docs/reference/Makefile
libmatenotify.pc
libmatenotify/Makefile
tests/Makefile
tools/Makefile
])

AC_OUTPUT

echo "
                    $PACKAGE $VERSION
                    ========================

	prefix:                   ${prefix}
	exec_prefix:              ${exec_prefix}
        bindir:                   ${bindir}
        datadir:                  ${datadir}
	source code location:	  ${srcdir}
	cflags:		          ${CFLAGS}

        Build docs:               ${enable_docbook_docs}

"
