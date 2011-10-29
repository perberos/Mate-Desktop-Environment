dnl Process this file with autoconf to create configure.

dnl ################################################################
dnl # Initialize autoconf
dnl ################################################################
AC_INIT([mate-notification-daemon], [2011.10.29], [https://github.com/perberos/mate-desktop-environment])
AC_PREREQ(2.63)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_MACRO_DIR([macros])

dnl ################################################################
dnl # Version information
dnl ################################################################
NOTIFICATION_DAEMON_MAJOR_VERSION=0
NOTIFICATION_DAEMON_MINOR_VERSION=5
NOTIFICATION_DAEMON_MICRO_VERSION=0
NOTIFICATION_DAEMON_DEVEL_VERSION=0

NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_MAJOR_VERSION.$NOTIFICATION_DAEMON_MINOR_VERSION.$NOTIFICATION_DAEMON_MICRO_VERSION

if test "x$NOTIFICATION_DAEMON_DEVEL_VERSION" != "x0"; then
	if test "x$NOTIFICATION_DAEMON_MICRO_VERSION" = "x0"; then
		NOTIFICATION_DAEMON_MICRO_VERSION=$NOTIFICATION_DAEMON_DEVEL_VERSION
		NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_MAJOR_VERSION.$NOTIFICATION_DAEMON_MINOR_VERSION.$NOTIFICATION_DAEMON_DEVEL_VERSION
	else
		NOTIFICATION_DAEMON_VERSION=$NOTIFICATION_DAEMON_VERSION.$NOTIFICATION_DAEMON_DEVEL_VERSION
	fi
fi

AC_DEFINE_UNQUOTED(NOTIFICATION_DAEMON_MAJOR_VERSION, $NOTIFICATION_DAEMON_MAJOR_VERSION,
	[mate-notification-daemon major version.])
AC_DEFINE_UNQUOTED(NOTIFICATION_DAEMON_MINOR_VERSION, $NOTIFICATION_DAEMON_MINOR_VERSION,
	[mate-notification-daemon minor version.])
AC_DEFINE_UNQUOTED(NOTIFICATION_DAEMON_MICRO_VERSION, $NOTIFICATION_DAEMON_MICRO_VERSION,
	[mate-notification-daemon micro version.])
AC_DEFINE_UNQUOTED(NOTIFICATION_DAEMON_VERSION, "$NOTIFICATION_DAEMON_VERSION",
	[mate-notification-daemon version.])

dnl ################################################################
dnl # Initialize automake
dnl ################################################################
AM_INIT_AUTOMAKE([1.10 no-dist-gzip dist-bzip2])
AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

dnl ################################################################
dnl # Check for some standard stuff.
dnl ################################################################
# Checks for programs
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_LN_S
AC_PROG_INSTALL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

dnl ---------------------------------------------------------------------------
dnl Initialize Libtool
dnl ---------------------------------------------------------------------------
LT_PREREQ([2.2.6])
LT_INIT


dnl ---------------------------------------------------------------------------
dnl i18n stuff
dnl ---------------------------------------------------------------------------
IT_PROG_INTLTOOL([0.40.0])

AC_SUBST([GETTEXT_PACKAGE], [mate-notification-daemon])
AM_GNU_GETTEXT_VERSION([0.11])
AM_GLIB_GNU_GETTEXT
AC_DEFINE([GETTEXT_PACKAGE], [PACKAGE_TARNAME], [Define to the gettext package name.])


dnl ---------------------------------------------------------------------------
dnl Requirements for the themes
dnl ---------------------------------------------------------------------------
GTK_API_VERSION=2.0
GTK_REQUIRED=2.18.0

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
       GTK_REQUIRED=3.0.0
       ;;
esac

PKG_CHECK_MODULES(THEME,
  gtk+-$GTK_API_VERSION >= $GTK_REQUIRED
)
AC_SUBST(THEME_CFLAGS)
AC_SUBST(THEME_LIBS)

dnl ---------------------------------------------------------------------------
dnl Requirements for the daemon
dnl ---------------------------------------------------------------------------
REQ_GLIB_VERSION=2.18.0
REQ_DBUS_VERSION=0.78
REQ_LIBCANBERRA_GTK_VERSION=0.4
pkg_modules="
	gtk+-$GTK_API_VERSION >= $GTK_REQUIRED, \
	glib-2.0 >= $REQ_GLIB_VERSION, \
	dbus-1 >= $REQ_DBUS_VERSION, \
	dbus-glib-1 >= $REQ_DBUS_VERSION, \
        libcanberra-gtk >= $REQ_LIBCANBERRA_GTK_VERSION, \
	mateconf-2.0, \
	libwnck-1.0 \
        x11 \
"
PKG_CHECK_MODULES(NOTIFICATION_DAEMON, $pkg_modules)
AC_SUBST(NOTIFICATION_DAEMON_CFLAGS)
AC_SUBST(NOTIFICATION_DAEMON_LIBS)

dnl ---------------------------------------------------------------------------
dnl Requirements for the setup tool
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES(NOTIFICATION_CAPPLET, glib-2.0 >= $REQ_GLIB_VERSION gtk+-$GTK_API_VERSION >= $GTK_REQUIRED mateconf-2.0 libmatenotify dbus-1 dbus-glib-1)
AC_SUBST(NOTIFICATION_CAPPLET_CFLAGS)
AC_SUBST(NOTIFICATION_CAPPLET_LIBS)

gdk_modules="
	gdk-2.0 >= $GTK_REQUIRED, \
	gdk-pixbuf-2.0 >= $GTK_REQUIRED \
"
PKG_CHECK_MODULES(GDK, $gdk_modules, have_gdk=yes,
[
	have_gdk=no
	AC_MSG_WARN("Some test apps will not be built")
])

AM_CONDITIONAL(HAVE_GDK, test "x$have_gdk" = "xyes")

AC_SUBST(GDK_CFLAGS)
AC_SUBST(GDK_LIBS)

AM_MATECONF_SOURCE_2
AC_PATH_PROG(MATECONFTOOL, mateconftool-2, no)

if test "x$MATECONFTOOL" = "xno"; then
	AC_MSG_ERROR([mateconftool-2 executable not found in your path - should be installed with Gconf])
fi

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

AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
AS_AC_EXPAND(LIBDIR, $libdir)
AS_AC_EXPAND(DATADIR, $datadir)
AS_AC_EXPAND(LIBEXECDIR, $libexecdir)

AC_ARG_WITH(dbus-sys, [  --with-dbus-sys=<dir>   where D-BUS system.d directory is])
AC_ARG_WITH(dbus-services, [  --with-dbus-services=<dir>   where D-BUS services directory is])

if ! test -z "$with_dbus_sys" ; then
	DBUS_SYS_DIR="$with_dbus_sys"
else
	DBUS_SYS_DIR="$SYSCONFDIR/dbus-1/system.d"
fi

if ! test -z "$with_dbus_services" ; then
	DBUS_SERVICES_DIR="$with_dbus_services"
else
	DBUS_SERVICES_DIR="$DATADIR/dbus-1/services"
fi

AC_SUBST(DBUS_SYS_DIR)
AC_SUBST(DBUS_SERVICES_DIR)
AC_DEFINE_UNQUOTED(DBUS_SYSTEMD_DIR, "$DBUS_SYS_DIR", [Where system.d dir for DBUS is])
AC_DEFINE_UNQUOTED(DBUS_SERVICES_DIR, "$DBUS_SERVICES_DIR", [Where services dir for DBUS is])

dnl ---------------------------------------------------------------------------
dnl Turn on the additional warnings last
dnl ---------------------------------------------------------------------------
MATE_COMPILE_WARNINGS([maximum])
MATE_CXX_WARNINGS([yes])
MATE_MAINTAINER_MODE_DEFINES

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
        WARNINGCFLAGS="\
        -Wchar-subscripts -Wmissing-declarations \
        -Wcast-align -Wsign-compare -Wp,-D_FORTIFY_SOURCE=2 \
        $WARNINGCFLAGS"

        for option in -Wno-strict-aliasing; do
                SAVE_CFLAGS="$WARNINGCFLAGS"
                WARNINGCFLAGS="$WARNINGCFLAGS $option"
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_TRY_COMPILE([], [],
                        has_option=yes,
                        has_option=no,)
                if test $has_option = no; then
                        WARNINGCFLAGS="$SAVE_CFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CFLAGS
        done
        unset option
else
        AC_MSG_RESULT(no)
fi

AM_CFLAGS="$AM_CFLAGS $WARN_CFLAGS $WARNINGCFLAGS"
AM_CXXFLAGS="$AM_CXXFLAGS $WARN_CXXFLAGS"

AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])

AC_CONFIG_FILES([
Makefile
data/Makefile
data/org.freedesktop.Notifications.service
po/Makefile.in
src/Makefile
src/daemon/Makefile
src/capplet/Makefile
src/themes/Makefile
src/themes/slider/Makefile
src/themes/standard/Makefile
])

AC_OUTPUT

echo "
			$PACKAGE_NAME $PACKAGE_VERSION
			=========================

	prefix:                   ${prefix}
	exec_prefix:              ${exec_prefix}
	bindir:                   ${bindir}
	datadir:                  ${datadir}
	source code location:     ${srcdir}
	cflags:		              ${AM_CFLAGS}

	dbus-1 system.d           $DBUS_SYS_DIR
	dbus-1 services           $DBUS_SERVICES_DIR

"
