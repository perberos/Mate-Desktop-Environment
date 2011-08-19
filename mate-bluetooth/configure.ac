AC_PREREQ(2.52)

m4_define(matebt_version_major, 2)
m4_define(matebt_version_minor, 32)
m4_define(matebt_version_micro, 0)

AC_INIT([mate-bluetooth],
        [matebt_version_major.matebt_version_minor.matebt_version_micro],
        [http://bugzilla.mate.org/enter_bug.cgi?product=mate-bluetooth])

AC_CONFIG_SRCDIR([applet/main.c])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.8 dist-bzip2 no-dist-gzip check-news])

# Before making a release, the MATEBT_LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
MATEBT_LT_VERSION=8:0:0
AC_SUBST(MATEBT_LT_VERSION)

AM_MAINTAINER_MODE

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_LANG_C
AC_PROG_CC
AC_PROG_INSTALL

dnl Initialize libtool
AM_DISABLE_STATIC
AM_PROG_LIBTOOL

GETTEXT_PACKAGE=mate-bluetooth2
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
					[The name of the gettext domain])

AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])
GLIB_GSETTINGS

AC_PATH_PROG([XMLLINT],[xmllint])

dnl gtk-doc checks
GTK_DOC_CHECK(1.9)

AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/share/locale", [Directory for the localization files])

AC_ARG_ENABLE(desktop-update, AC_HELP_STRING([--disable-desktop-update],
					[Disable desktop database update]))
if (test "$enable_desktop_update" != no); then
	AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, [update-desktop-database])
fi
AM_CONDITIONAL([DESKTOP_UPDATE], [test -n "$UPDATE_DESKTOP_DATABASE"])

AC_ARG_ENABLE(icon-update, AC_HELP_STRING([--disable-icon-update],
					[Disable icon cache update]))
if (test "$enable_icon_update" != no); then
	AC_PATH_PROG(UPDATE_ICON_CACHE, [gtk-update-icon-cache])
fi
AM_CONDITIONAL([ICON_UPDATE], [test -n "$UPDATE_ICON_CACHE"])

GTK_REQUIRED=2.19.1
GLIB_REQUIRED=2.25.7
DBUS_GLIB_REQUIRED=0.74
NOTIFY_REQUIRED=0.4.3
CAJA_SENDTO_REQUIRED=2.31.7

dnl Requires for the properties window
PKG_CHECK_MODULES(PROPS,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  glib-2.0 >= $GLIB_REQUIRED
		  gtk+-2.0
		  unique-1.0)

dnl Requires for the applet
PKG_CHECK_MODULES(APPLET,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  gmodule-export-2.0
		  glib-2.0 >= $GLIB_REQUIRED
		  gtk+-2.0 >= $GTK_REQUIRED
		  libmatenotify >= $NOTIFY_REQUIRED
		  unique-1.0)

dnl Requires for the sendto app
PKG_CHECK_MODULES(SENDTO,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  gtk+-2.0
		  gio-2.0)

dnl caja-sendto plugin
AC_ARG_ENABLE(caja-sendto,
              AS_HELP_STRING([--enable-caja-sendto=@<:@no/yes/auto@:>@],
                             [build caja-sendto plugin]), ,
                             enable_nst=yes)

if test "x$enable_nst" != "xno"; then
   PKG_CHECK_MODULES(NST,
   [
      caja-sendto >= $CAJA_SENDTO_REQUIRED
   ], have_nst="yes", have_nst="no")
else
   have_nst=no
fi

if test "x$enable_nst" = "xyes" -a "x$have_nst" != "xyes"; then
   AC_MSG_ERROR([Couldn't find caja-sendto dependencies.])
fi

AM_CONDITIONAL(HAVE_NST, test "x$have_nst" = "xyes")

dnl Requires for the wizard dialogue
PKG_CHECK_MODULES(WIZARD,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  gmodule-export-2.0
		  gtk+-2.0
		  unique-1.0)

dnl Requires for the input helper
PKG_CHECK_MODULES(INPUT,
		  xi
		  x11
		  gtk+-2.0)

dnl Requires for the public library
PKG_CHECK_MODULES(LIBMATEBT,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  gtk+-2.0)

dnl Requires for the private library
PKG_CHECK_MODULES(COMMON,
		  dbus-glib-1 >= $DBUS_GLIB_REQUIRED
		  gmodule-2.0
		  gtk+-2.0)

dnl Requires for the geoclue plugin
PKG_CHECK_MODULES(GEOCLUE,
		  mateconf-2.0
		  dbus-glib-1)

dnl Requires for the plugins
PKG_CHECK_MODULES(PLUGINS,
		  gtk+-2.0)

dnl Requires for the moblin interface
AC_ARG_ENABLE(moblin,
              AS_HELP_STRING([--enable-moblin=@<:@no/yes/auto@:>@],
                             [build moblin interface]), ,
                             enable_moblin=auto)

if test "x$enable_moblin" != "xno"; then
   PKG_CHECK_MODULES(MOBLIN,
   [
      dbus-glib-1,
      gtk+-2.0,
      mx-gtk-1.0
      moblin-panel
   ], have_moblin="yes", have_moblin="no")

   THEME_DIR="${datadir}/${PACKAGE}/theme"
   AS_AC_EXPAND(THEME_DIR, $THEME_DIR)
   AC_DEFINE_UNQUOTED([THEME_DIR], "$THEME_DIR", [Carrick theme directory])

   PANEL_DIR=$($PKG_CONFIG moblin-panel --variable=moblin_panel_panels_dir)
   AC_SUBST(PANEL_DIR)
else
   have_moblin=no
fi

if test "x$enable_moblin" = "xyes" -a "x$have_moblin" != "xyes"; then
   AC_MSG_ERROR([Couldn't find Moblin dependencies.])
fi
AM_CONDITIONAL(WITH_MOBLIN, test "x$have_moblin" = "xyes")

DBUS_BINDING_TOOL="dbus-binding-tool"
AC_SUBST(DBUS_BINDING_TOOL)

GLIB_GENMARSHAL=`$PKG_CONFIG --variable=glib_genmarshal glib-2.0`
AC_SUBST(GLIB_GENMARSHAL)

GOBJECT_INTROSPECTION_CHECK([0.6.7])

MATE_COMMON_INIT
MATE_DEBUG_CHECK
MATE_COMPILE_WARNINGS([maximum])
MATE_CXX_WARNINGS
MATE_MAINTAINER_MODE_DEFINES

MATE_DOC_INIT

AC_OUTPUT(Makefile
	  mate-bluetooth-1.0.pc
	  icons/Makefile
	  lib/Makefile
	  lib/plugins/Makefile
	  applet/Makefile
	  applet/bluetooth-applet.desktop.in
	  properties/Makefile
	  properties/bluetooth-properties.desktop.in
	  wizard/Makefile
	  sendto/Makefile
	  moblin/Makefile
	  moblin/bluetooth-panel.desktop.in
	  moblin/theme/Makefile
	  moblin/moblin-copy-n-paste/Makefile
	  moblin/moblin-icons/Makefile
	  docs/Makefile
	  docs/reference/Makefile
	  docs/reference/libmate-bluetooth/Makefile
	  docs/reference/libmate-bluetooth/version.xml
	  po/Makefile.in
	  help/Makefile
)

echo "
Configure summary:

	Compiler....................:  ${CC}
	Compiler Flags..............:  ${CFLAGS}
	Prefix......................:  ${prefix}
	Moblin interface............:  ${have_moblin}
	caja-sendto plugin......:  ${have_nst}
	Documentation...............:  ${enable_gtk_doc}
	GObject-Introspection.......:  ${found_introspection}
"

