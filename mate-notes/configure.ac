AC_INIT(gnote, 0.7.4)
AC_PREREQ(2.59)
AC_CONFIG_SRCDIR(README)
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

MATE_COMMON_INIT
MATE_DOC_INIT

AC_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR(m4)

m4_pattern_allow([^BOOST_])

MAJOR_VERSION=0
MINOR_VERSION=7
MICRO_VERSION=4

GNOTE_VERSION="$MAJOR_VERSION.$MINOR_VERSION.$MICRO_VERSION"
AC_SUBST(GNOTE_VERSION)


dnl all the library version.
dnl if one is harcoded elsewhere, it is a bug

GTK_VERSION=2.20
LIBGLIBMM_VERSION=2.18
LIBGTKMM_VERSION=2.12.0
LIBMATECONF_VERSION=2.0.0
LIBPANELAPPLETMM_VERSION=2.22.0
GTKSPELL_VERSION=2.0.9
BOOST_VERSION=1.34
DBUSCPP_VERSION=0.5.0

AC_PROG_CXX
AC_GNU_SOURCE

AC_ARG_ENABLE(debug,[  --enable-debug    Turn on debugging],[
        case "${enableval}" in
           yes) debug=true ;
                DEBUG_CFLAGS="-DDEBUG -g" ;
                OPTIMIZE_CFLAGS="" ;;
            no) debug=false ;
                DEBUG_CFLAGS="-DNDEBUG" ;;
             *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
        esac
],[     debug=false
        DEBUG_CFLAGS="-DNDEBUG"
])
AM_CONDITIONAL(DEBUG, test x$debug = xtrue)


CPPFLAGS="$CPPFLAGS $DEBUG_CFLAGS -Wall -Wcast-align -Wcast-qual -Wpointer-arith -Wreturn-type"
CFLAGS="$CFLAGS $DEBUG_CFLAGS $OPTIMIZE_CFLAGS"
CXXFLAGS="$CXXFLAGS $DEBUG_CFLAGS $OPTIMIZE_CFLAGS"
dnl CFLAGS=""
LDFLAGS="$LDFLAGS"


dnl AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_LANG_CPLUSPLUS
AC_LANG_COMPILER_REQUIRE

PKG_CHECK_MODULES(LIBGLIBMM, [glibmm-2.4])
PKG_CHECK_MODULES(GTK, [gtk+-2.0 >= $GTK_VERSION])
PKG_CHECK_MODULES(LIBGTKMM, [gtkmm-2.4 >= $LIBGTKMM_VERSION])
PKG_CHECK_MODULES(LIBXML, [libxml-2.0])
PKG_CHECK_MODULES(LIBXSLT, [libxslt])
PKG_CHECK_MODULES(MATECONF, [mateconf-2.0])
PKG_CHECK_MODULES(PCRE, [libpcrecpp])

PKG_CHECK_EXISTS(gtkmm-2.4 >= 2.14.0,
  [AC_DEFINE(HAVE_CLASS_GTK__WIDGET_SIGNAL_POPUP_MENU, 1, [Define to 1 if class Gtk::Widget has signal_popup_menu])],
  [])

dnl check for the ext2 uuid library. TODO replace it with something more portable
AC_CHECK_HEADER(uuid/uuid.h,,[AC_MSG_ERROR("uuid header is missing")])
AC_CHECK_LIB(uuid,uuid_unparse_lower, ,[AC_MSG_ERROR("libuuid library is missing")])

AC_ARG_ENABLE([applet],
	AS_HELP_STRING([--disable-applet],[Disable panel applet]),
	[],[enable_applet=yes])

if test $enable_applet = "yes"; then
	PKG_CHECK_EXISTS(libmatepanelappletmm-2.6 >= $LIBPANELAPPLETMM_VERSION,
	  [
		AC_DEFINE(HAVE_PANELAPPLETMM, 1, [Define to 1 if matepanelapplet is available])
		have_matepanelapplet="yes"
		PKG_CHECK_MODULES(LIBPANELAPPLETMM, [libmatepanelappletmm-2.6 >= $LIBPANELAPPLETMM_VERSION])
	  ], [
		have_matepanelapplet="no"]
	)

	#
	# If libmatepanelapplet available, use it to find .server install location.
	#
	if test "x$prefix" = "x$ac_default_prefix"; then
		PANELAPPLET_LIBDIR=`$PKG_CONFIG --variable=libdir libmatepanelapplet-2.0`
	fi
	if test "x$PANELAPPLET_LIBDIR" != "x"; then
		PANELAPPLET_SERVER_DIR="$PANELAPPLET_LIBDIR/matecomponent/servers"
	fi
	# Set PANELAPPLET_SERVER_DIR default if it's not already set
	if test "x$PANELAPPLET_SERVER_DIR" = "x" ; then
		PANELAPPLET_SERVER_DIR="$libdir/matecomponent/servers"
	fi
else
	have_matepanelapplet="no"
fi

AM_CONDITIONAL(HAVE_PANELAPPLETMM, test $have_matepanelapplet = "yes")
AC_SUBST(PANELAPPLET_SERVER_DIR)

#
# Check for recent GtkSpell here.  Prior versions require bug workarounds.
# http://gtkspell.sourceforge.net/
#
PKG_CHECK_MODULES(GTKSPELL, gtkspell-2.0 >= $GTKSPELL_VERSION, 
			    [FIXED_GTKSPELL="yes";AC_DEFINE([FIXED_GTKSPELL], [1], [Set if the GtkSpell library is recent enough.])
			    ], FIXED_GTKSPELL="no")
AM_CONDITIONAL(FIXED_GTKSPELL, test "$FIXED_GTKSPELL" = "yes")



BOOST_REQUIRE([$BOOST_VERSION])
BOOST_BIND
BOOST_CONVERSION
BOOST_FORMAT
BOOST_TEST([s])
AC_CHECK_HEADER(tr1/memory,,[
	CXXFLAGS="$CXXFLAGS -I/usr/include/boost/tr1";
	AC_MSG_NOTICE([using boost tr1 implementation.])
])


AC_LANG_PUSH(C++)
if test "$GCC" = "yes"; then
        GNOTE_BUILD_CONFIG="$GNOTE_BUILD_CONFIG gcc-options="
        for option in -Wall -Wextra -Wsign-compare -Wpointer-arith \
                      -Wchar-subscripts -Wwrite-strings \
                      -Wunused -Wpointer-arith -Wshadow -fshow-column ; do
                SAVE_CXXFLAGS="$CXXFLAGS"
                CXXFLAGS="$CXXFLAGS $option"
		GNOTE_BUILD_CONFIG="$GNOTE_BUILD_CONFIG$option "
                AC_MSG_CHECKING([whether gcc understands $option])
                AC_TRY_COMPILE([], [],
                        has_option=yes,
                        has_option=no,)
                if test $has_option = no; then
                  CXXFLAGS="$SAVE_CXXFLAGS"
                fi
                AC_MSG_RESULT($has_option)
                unset has_option
                unset SAVE_CXXFLAGS
        done
        unset option
fi
AC_LANG_POP

#
# Use DBus to export our remote control if available.
#
AC_ARG_ENABLE(dbus,
	AS_HELP_STRING([--enable-dbus],[compile with dbus support @<:@default: yes@:>@]),
	[],[enable_dbus=yes])

if test "$enable_dbus" != "no"; then
	PKG_CHECK_MODULES(DBUS, dbus-c++-1 >= $DBUSCPP_VERSION, [
		have_dbus="yes"
		AC_DEFINE([ENABLE_DBUS], [1], [Define to 1 if DBus is enabled])
	], have_dbus="no")
fi
AM_CONDITIONAL(HAVE_DBUS, test "$have_dbus" = "yes")



AC_ARG_WITH(dbus_service_dir, [  --with-dbus-service-dir=DIR            Where to install Gnote's DBus service file.])
AM_CONDITIONAL(WITH_DBUS_SERVICE_DIR, test "x$with_dbus_service_dir" != "x")
if test "x$with_dbus_service_dir" != "x"; then
	DBUS_SERVICE_DIR=$with_dbus_service_dir
else
	DBUS_SERVICE_DIR=${datadir}/dbus-1/services
fi
AC_SUBST(DBUS_SERVICE_DIR)


IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=gnote
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"],
                   [The domain to use with gettext])
AM_GLIB_GNU_GETTEXT

#
# Find mateconftool-2
#
AC_PATH_PROG(MATECONFTOOL, mateconftool-2)
AM_MATECONF_SOURCE_2

GNOTE_LOCALEDIR=[${datadir}/locale]
AC_SUBST(GNOTE_LOCALEDIR)

AC_DEFINE_UNQUOTED([GNOTE_BUILD_CONFIG], ["$GNOTE_BUILD_CONFIG"], [The string used to hardcode the build config.])

SHAVE_INIT

AC_CONFIG_FILES([
Makefile
shave
shave-libtool
data/Makefile
data/gnote.desktop.in
data/icons/Makefile
libtomboy/Makefile
src/Makefile
src/addins/Makefile
src/addins/addins.mk
src/addins/backlinks/Makefile
src/addins/bugzilla/Makefile
src/addins/exporttohtml/Makefile
src/addins/fixedwidth/Makefile
src/addins/inserttimestamp/Makefile
src/addins/noteoftheday/Makefile
src/addins/printnotes/Makefile
src/addins/stickynoteimport/Makefile
src/addins/tomboyimport/Makefile
src/addins/underline/Makefile
src/dbus/Makefile
po/Makefile.in
po/Makefile
help/Makefile
])


AC_OUTPUT


echo "
Options:

  Spellchecking support:       ${FIXED_GTKSPELL}
  MATE Panel Applet support:  ${have_matepanelapplet}
  DBus support:                ${have_dbus}

"
