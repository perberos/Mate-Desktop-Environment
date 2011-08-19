AC_PREREQ(2.63)

AC_INIT([mate-power-manager],[2.32.0])
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AM_PROG_CC_C_O

AC_PROG_INSTALL
LT_INIT
IT_PROG_INTLTOOL([0.35.0])

MATE_COMPILE_WARNINGS
MATE_DOC_INIT

AC_PATH_PROG(GLIB_GENMARSHAL, glib-genmarshal)

dnl ---------------------------------------------------------------------------
dnl - Use strict options (default enabled for devs, disabled in releases)
dnl ---------------------------------------------------------------------------
dnl if .git directory is present, considering we are working in the repository
if test -d ".git"; then
	default_strict=yes
else
	default_strict=no
fi

AC_ARG_ENABLE(strict, AS_HELP_STRING([--enable-strict],
		[Enable strict compilation options]), enable_strict=$enableval,
		enable_strict=$default_strict)
if test x$enable_strict != xno; then
	CPPFLAGS="$CPPFLAGS -DG_DISABLE_SINGLE_INCLUDES -DGTK_DISABLE_SINGLE_INCLUDES"
	CPPFLAGS="$CPPFLAGS -DG_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED"
	CPPFLAGS="$CPPFLAGS -DGDK_MULTIHEAD_SAFE -DGTK_MULTIHEAD_SAFE"
	CPPFLAGS="$CPPFLAGS -DGSEAL_ENABLE"
	if test "$GCC" = "yes"; then
		WARNINGFLAGS="$WARNINGFLAGS -Werror"
	fi
fi

if test "$GCC" = "yes"; then
	WARNINGFLAGS="$WARNINGFLAGS -Wall"
	WARNINGFLAGS="$WARNINGFLAGS -Wcast-align -Wno-uninitialized"
	WARNINGFLAGS="$WARNINGFLAGS -Wmissing-declarations"
#	WARNINGFLAGS="$WARNINGFLAGS -Wredundant-decls"
#	WARNINGFLAGS="$WARNINGFLAGS -Wmissing-noreturn"
	WARNINGFLAGS="$WARNINGFLAGS -Wpointer-arith"
	WARNINGFLAGS="$WARNINGFLAGS -Wcast-align"
	WARNINGFLAGS="$WARNINGFLAGS -Wwrite-strings"
	WARNINGFLAGS="$WARNINGFLAGS -Winit-self"
	WARNINGFLAGS="$WARNINGFLAGS -Wreturn-type"
	WARNINGFLAGS="$WARNINGFLAGS -Wformat-nonliteral"
	WARNINGFLAGS="$WARNINGFLAGS -Wformat-security"
	WARNINGFLAGS="$WARNINGFLAGS -Wmissing-include-dirs"
	WARNINGFLAGS="$WARNINGFLAGS -Wmissing-format-attribute"
	WARNINGFLAGS="$WARNINGFLAGS -Wclobbered"
	WARNINGFLAGS="$WARNINGFLAGS -Wempty-body"
	WARNINGFLAGS="$WARNINGFLAGS -Wignored-qualifiers"
#	WARNINGFLAGS="$WARNINGFLAGS -Wsign-compare"
	WARNINGFLAGS="$WARNINGFLAGS -Wtype-limits"
#	WARNINGFLAGS="$WARNINGFLAGS -Wuninitialized"
#	WARNINGFLAGS="$WARNINGFLAGS -Waggregate-return"
	WARNINGFLAGS="$WARNINGFLAGS -Wdeclaration-after-statement"
	WARNINGFLAGS="$WARNINGFLAGS -Wshadow"
	WARNINGFLAGS="$WARNINGFLAGS -Winline"
#	WARNINGFLAGS="$WARNINGFLAGS -Wswitch-enum"
	WARNINGFLAGS="$WARNINGFLAGS -Wmissing-parameter-type"
	WARNINGFLAGS="$WARNINGFLAGS -Woverride-init"
	WARNINGFLAGS="$WARNINGFLAGS -Wno-strict-aliasing"
else
	WARNINGFLAGS=""
fi
AC_SUBST(WARNINGFLAGS)

dnl ---------------------------------------------------------------------------
dnl - Debugging switches (uncomment this if you want to use gdb)
dnl ---------------------------------------------------------------------------
CPPFLAGS="$CPPFLAGS -g"
CPPFLAGS="$CPPFLAGS -fexceptions"

dnl ---------------------------------------------------------------------------
dnl - gettext stuff
dnl ---------------------------------------------------------------------------
GETTEXT_PACKAGE=AC_PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE(GETTEXT_PACKAGE, "AC_PACKAGE_NAME", [foo])

AM_GLIB_GNU_GETTEXT

dnl ---------------------------------------------------------------------------
dnl - Library dependencies
dnl ---------------------------------------------------------------------------
GLIB_REQUIRED=2.13.0
GTK_REQUIRED=2.17.7
GDK_REQUIRED=2.11.0
MATECONF_REQUIRED=2.10.0
MATE_KEYRING_REQUIRED=0.6.0
DBUS_REQUIRED=1.0
DBUS_GLIB_REQUIRED=0.70
LIBMATENOTIFY_REQUIRED=0.4.3
LIBWNCK_REQUIRED=2.10.0
CAIRO_REQUIRED=1.0.0
UNIQUE_REQUIRED=0.9.4
LIBPANEL_REQUIRED=2.0.0
XRANDR_REQUIRED=1.2.0
CANBERRA_REQUIRED=0.10
UPOWER_REQUIRED=0.9.1

dnl ---------------------------------------------------------------------------
dnl - Check library dependencies
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED gobject-2.0)

PKG_CHECK_MODULES(CANBERRA, libcanberra-gtk >= $CANBERRA_REQUIRED)

PKG_CHECK_MODULES(DBUS,[
 dbus-glib-1 >= $DBUS_GLIB_REQUIRED
 dbus-1 >= $DBUS_REQUIRED
 gthread-2.0])

PKG_CHECK_MODULES(MATE, [
 libwnck-1.0 >= $LIBWNCK_REQUIRED
 gtk+-2.0 >= $GTK_REQUIRED
 mateconf-2.0 >= $MATECONF_REQUIRED
 mate-keyring-1 >= $MATE_KEYRING_REQUIRED
 cairo >= $CAIRO_REQUIRED])

PKG_CHECK_MODULES(GDK, [
 gdk-2.0 >= $GDK_REQUIRED
 gdk-x11-2.0 >= $GDK_REQUIRED])

PKG_CHECK_MODULES(X11, [
 xrandr >= $XRANDR_REQUIRED
 xrender x11 xext xproto])

PKG_CHECK_MODULES(X11, x11 xrender)

PKG_CHECK_EXISTS(
 [xrandr >= 1.3],
 [AC_DEFINE(HAVE_XRANDR_13, 1, [xrandr 1.3 available])])

PKG_CHECK_MODULES(LIBMATENOTIFY, libmatenotify >= $LIBMATENOTIFY_REQUIRED)

PKG_CHECK_MODULES(UNIQUE, unique-1.0 >= $UNIQUE_REQUIRED)

PKG_CHECK_MODULES(UPOWER, upower-glib >= $UPOWER_REQUIRED)

AC_PATH_PROG(MATECONFTOOL, mateconftool-2)
AM_MATECONF_SOURCE_2

dnl ---------------------------------------------------------------------------
dnl - Check XF86XK_Keys
dnl ---------------------------------------------------------------------------
PKG_CHECK_EXISTS(
 [xproto >= 7.0.14],
 [AC_DEFINE(HAVE_XF86XK_BATTERY, 1, [Define if XF86XK_Battery is available])])

PKG_CHECK_EXISTS(
 [xproto >= 7.0.15],
 [AC_DEFINE(HAVE_XF86XK_SUSPEND, 1, [Define if XF86XK_Suspend is available])
  AC_DEFINE(HAVE_XF86XK_HIBERNATE, 1, [Define if XF86XK_Hibernate is available])])

dnl ---------------------------------------------------------------------------
dnl - Make paths available for source files
dnl ---------------------------------------------------------------------------
AC_SUBST(SYSCONFDIR, $sysconfdir)
AC_SUBST(DATADIR, $datadir)
AC_SUBST(BINDIR, $bindir)
AC_SUBST(SBINDIR, $sbindir)

dnl ---------------------------------------------------------------------------
dnl - Where should we put documentation ?
dnl ---------------------------------------------------------------------------
AC_ARG_WITH(doc-dir,
            AS_HELP_STRING([--with-doc-dir=<dir>],
                           [directory to install documentation]),
            [DOCDIR="$with_doc_dir/mate-power-manager-$VERSION"],
            [DOCDIR="$DATADIR/doc/mate-power-manager-$VERSION"])
AC_SUBST(DOCDIR)

dnl ---------------------------------------------------------------------------
dnl - Build self tests
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(tests,
              AS_HELP_STRING([--enable-tests],
                             [Build self tests]),
              enable_tests=$enableval,enable_tests=no)

AC_MSG_CHECKING([whether to support tests])
have_tests=no
if test x$enable_tests = xyes ; then
    have_tests=yes
    AC_DEFINE(HAVE_TESTS, 1, [Define if we want to use the self tests])
    AC_MSG_RESULT(yes)
else
    AC_MSG_RESULT(no)
fi
AM_CONDITIONAL([HAVE_TESTS], [test $have_tests = yes])

dnl ---------------------------------------------------------------------------
dnl - Build mateconf default button?
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(mateconf-defaults,
              AS_HELP_STRING([--disable-mateconf-defaults],
                             [Allow setting mateconf default settings]),
              have_mateconfdefaults=$enableval,have_mateconfdefaults=yes)

AC_MSG_CHECKING([whether to support mateconf defaults])
if test x$have_mateconfdefaults = xyes ; then
    AC_DEFINE(HAVE_MATECONF_DEFAULTS, 1, [Define if have mateconf defaults])
fi
AC_MSG_RESULT([$have_mateconfdefaults])
AM_CONDITIONAL([HAVE_MATECONF_DEFAULTS], [test $have_mateconfdefaults = yes])

dnl ---------------------------------------------------------------------------
dnl - Build applets
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(applets,
              AS_HELP_STRING([--disable-applets],
                             [Build the extra power applets]),
              enable_applets=$enableval,enable_applets=yes)

AC_MSG_CHECKING([whether to build applet support])
have_applets=no
if test x$enable_applets = xyes ; then
    have_applets=yes

    PKG_CHECK_MODULES(PANEL, [libmatepanelapplet-2.0 >= $LIBPANEL_REQUIRED])
    AC_DEFINE(HAVE_APPLETS, 1, [Define if we are building applets])
fi
AC_MSG_RESULT([$have_applets])
AM_CONDITIONAL([HAVE_APPLETS], [test $have_applets = yes])

dnl ---------------------------------------------------------------------------
dnl - DocBook Documentation
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(docbook-docs,
              AS_HELP_STRING([--enable-docbook-docs],
                             [build documentation (requires xmlto)]),
              enable_docbook_docs=$enableval,enable_docbook_docs=auto)

AC_PATH_PROG(XMLTO, xmlto, no)
AC_MSG_CHECKING([whether to build DocBook documentation])
if test x$enable_docbook_docs = xauto ; then
    if test x$XMLTO = xno ; then
        enable_docbook_docs=no
    else
        enable_docbook_docs=yes
    fi
fi
if test x$enable_docbook_docs = xyes -a x$XMLTO = xno; then
	AC_MSG_ERROR([Building DocBook docs explicitly required, but DocBook not found])
fi
AM_CONDITIONAL(DOCBOOK_DOCS_ENABLED, test x$enable_docbook_docs = xyes)
AC_MSG_RESULT(yes)

dnl ---------------------------------------------------------------------------
dnl - Are we specifying a different dbus root ?
dnl ---------------------------------------------------------------------------
AC_ARG_WITH(dbus-services,
            AS_HELP_STRING([--with-dbus-services=<dir>],
                           [where D-BUS services directory is]),
            [DBUS_SERVICES_DIR="$with_dbus_services"],
            [DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"])
AC_SUBST(DBUS_SERVICES_DIR)

dnl ---------------------------------------------------------------------------
dnl - Check for Solaris kstat support
dnl ---------------------------------------------------------------------------
AC_MSG_CHECKING(for Solaris kstat)
AC_CHECK_LIB(kstat, kstat_open,
	     [
	       AC_DEFINE(HAVE_KSTAT, 1, [Define if Solaris kstat is support])
	       GPM_EXTRA_LIBS="$GPM_EXTRA_LIBS -lkstat"
	     ])
AC_SUBST(GPM_EXTRA_LIBS)

dnl ---------------------------------------------------------------------------
dnl - Is docbook2man available?
dnl ---------------------------------------------------------------------------
AC_PATH_PROG(DOCBOOK2MAN, docbook2man, no)
if test "$DOCBOOK2MAN" = "no" ; then
	AC_MSG_WARN([docbook2man not found, will not be able to build man documentation])
fi
AM_CONDITIONAL(HAVE_DOCBOOK2MAN, [test "$DOCBOOK2MAN" != "no"])

dnl ---------------------------------------------------------------------------
dnl - Makefiles, etc.
dnl ---------------------------------------------------------------------------
AC_CONFIG_FILES([
Makefile
src/Makefile
applets/Makefile
applets/brightness/Makefile
applets/inhibit/Makefile
docs/Makefile
man/Makefile
help/Makefile
po/Makefile.in
tools/Makefile
policy/Makefile
data/Makefile
data/mate-power-manager.desktop.in
data/mate-power-preferences.desktop.in
data/mate-power-statistics.desktop.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/16x16/actions/Makefile
data/icons/16x16/apps/Makefile
data/icons/16x16/status/Makefile
data/icons/22x22/Makefile
data/icons/22x22/actions/Makefile
data/icons/22x22/apps/Makefile
data/icons/22x22/status/Makefile
data/icons/24x24/Makefile
data/icons/24x24/actions/Makefile
data/icons/24x24/apps/Makefile
data/icons/24x24/status/Makefile
data/icons/32x32/Makefile
data/icons/32x32/actions/Makefile
data/icons/32x32/apps/Makefile
data/icons/32x32/status/Makefile
data/icons/48x48/Makefile
data/icons/48x48/actions/Makefile
data/icons/48x48/apps/Makefile
data/icons/48x48/status/Makefile
data/icons/scalable/Makefile
data/icons/scalable/actions/Makefile
data/icons/scalable/apps/Makefile
data/icons/scalable/status/Makefile
])

AC_OUTPUT

dnl ==========================================================================
echo "
                    MATE Power Manager $VERSION
                  =============================

        prefix:                    ${prefix}
        datadir:                   ${datadir}
        compiler:                  ${CC}
        cflags:                    ${CFLAGS}
        Building extra applets:    ${enable_applets}
        Self test support:         ${have_tests}
        MateConf default support:     ${have_mateconfdefaults}
        Docbook support:           ${enable_docbook_docs}
        documentation dir:         $DOCDIR
        dbus-1 services dir:       $DBUS_SERVICES_DIR
        mateconf-schema dir:          $MATECONF_SCHEMA_FILE_DIR
"
if [[ "${prefix}" != "/usr" ]] ; then
	echo '
WARNING!!!  MATE Power Manager uses the "pkexec" utility to provide root
permissions necessary for the "mate-power-backlight-helper" executable to run.

A link should be provided from the file
"/usr/share/polkit-1/actions/org.mate.power.policy" to the installed version
"${prefix}/share/polkit-1/actions/org.mate.power.policy" after installation.
'
fi

