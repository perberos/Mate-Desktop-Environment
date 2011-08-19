AC_INIT([MATE Disk Utility],[2.32.0],[https://bugzilla.mate.org/enter_bug.cgi?product=mate-disk-utility],[mate-disk-utility])

AM_INIT_AUTOMAKE([1.9 foreign dist-bzip2 no-dist-gzip])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE

# libtool versioning - this applies to all libraries in this package
#
# See http://sources.redhat.com/autobook/autobook/autobook_91.html#SEC91 for details
#
LT_CURRENT=0
LT_REVISION=0
LT_AGE=0
AC_SUBST(LT_CURRENT)
AC_SUBST(LT_REVISION)
AC_SUBST(LT_AGE)

AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AC_PROG_LIBTOOL

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

#### gcc warning flags

AC_ARG_ENABLE(ansi, AS_HELP_STRING([--enable-ansi],
  [enable -ansi -pedantic gcc flags]),
  enable_ansi=$enableval,enable_ansi=no)

if test "x$GCC" = "xyes"; then
  changequote(,)dnl
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wchar-subscripts[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wchar-subscripts" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wmissing-declarations[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wmissing-declarations" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wnested-externs[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wnested-externs" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wpointer-arith[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wpointer-arith" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wcast-align[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wcast-align" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wsign-compare[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wsign-compare" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wformat[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wformat" ;;
  esac

  case " $CFLAGS " in
  *[\ \	]-Wformat-security[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wformat-security" ;;
  esac

  if test "x$enable_ansi" = "xyes"; then
    case " $CFLAGS " in
    *[\ \	]-ansi[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -ansi" ;;
    esac

    case " $CFLAGS " in
    *[\ \	]-D_POSIX_C_SOURCE*) ;;
    *) CFLAGS="$CFLAGS -D_POSIX_C_SOURCE=199309L" ;;
    esac

    case " $CFLAGS " in
    *[\ \	]-D_BSD_SOURCE[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -D_BSD_SOURCE" ;;
    esac

    case " $CFLAGS " in
    *[\ \	]-pedantic[\ \	]*) ;;
    *) CFLAGS="$CFLAGS -pedantic" ;;
    esac
  fi
  changequote([,])dnl
fi

# *****
# MATE
# *****

MATE_COMMON_INIT
MATE_DOC_INIT
MATE_DEBUG_CHECK
MATE_COMPILE_WARNINGS([maximum])
MATE_MAINTAINER_MODE_DEFINES

AC_ARG_ENABLE(gtk-doc, AS_HELP_STRING([--enable-gtk-doc],
  [use gtk-doc to build documentation [default=no]]),,
  enable_gtk_doc=no)
GTK_DOC_CHECK([1.3])

# ***************************
# Check for required packages
# ***************************

GLIB2_REQUIRED=2.22
GOBJECT2_REQUIRED=2.16
GIO2_REQUIRED=2.22
DBUS_GLIB_REQUIRED=0.74
MATE_KEYRING_REQUIRED=2.22
GTK2_REQUIRED=2.20.0
UNIQUE_REQUIRED=1.0
LIBMATENOTIFY_REQUIRED=0.3.0
CAJA_REQUIRED=2.24.0
AVAHI_UI_REQUIRED=0.6.25

UDISKS_REQUIRED=1.0.0
UDISKS_NEXT_ABI_INCOMPATIBLE_VERSION=1.1.0

SCROLLKEEPER_REQUIRED=0.3.14
INTLTOOL_REQUIRED=0.35.0

PKG_CHECK_MODULES(GLIB2, [glib-2.0 >= $GLIB2_REQUIRED])
PKG_CHECK_MODULES(GOBJECT2, [gobject-2.0 >= $GOBJECT2_REQUIRED])
PKG_CHECK_MODULES(GIO2, [gio-2.0 >= $GIO2_REQUIRED])
PKG_CHECK_MODULES(GIO_UNIX2, [gio-unix-2.0 >= $GIO2_REQUIRED])
PKG_CHECK_MODULES(GTHREAD2, [gthread-2.0 >= $GLIB2_REQUIRED])
PKG_CHECK_MODULES(DBUS_GLIB, [dbus-glib-1 >= $DBUS_GLIB_REQUIRED])
PKG_CHECK_MODULES(GTK2, [gtk+-2.0 >= $GTK2_REQUIRED])
PKG_CHECK_MODULES(UNIQUE, [unique-1.0 >= $UNIQUE_REQUIRED])
PKG_CHECK_MODULES(LIBMATENOTIFY, [libmatenotify  >= $LIBMATENOTIFY_REQUIRED])
PKG_CHECK_MODULES(UDISKS, [udisks  >= $UDISKS_REQUIRED udisks < $UDISKS_NEXT_ABI_INCOMPATIBLE_VERSION])
PKG_CHECK_MODULES(X11, [x11])
PKG_CHECK_MODULES(LIBATASMART, [libatasmart >= 0.14])
PKG_CHECK_MODULES(AVAHI_UI, [avahi-ui >= $AVAHI_UI_REQUIRED])

# *************
# Remote Access
# *************
have_remote_access=no
AC_ARG_ENABLE(remote_access, AS_HELP_STRING([--disable-remote-access], [disable access to remote udisks daemons]))
if test "x$enable_remote_access" != "xno"; then
   AC_DEFINE(HAVE_REMOTE_ACCESS, 1, [Defined if remote access is enabled])
   have_remote_access=yes
fi
AM_CONDITIONAL(HAVE_REMOTE_ACCESS, [test "$have_remote_access" = "yes"])

# *************
# MATE Keyring
# *************

have_mate_keyring=no
AC_ARG_ENABLE(mate-keyring, AS_HELP_STRING([--disable-mate-keyring], [disable use of MATE Keyring]))

if test "x$enable_mate_keyring" != "xno"; then
	PKG_CHECK_MODULES(MATE_KEYRING, mate-keyring-1 >= $MATE_KEYRING_REQUIRED,
			  [AC_DEFINE(HAVE_MATE_KEYRING, 1, [Define if mate-keyring is available])
			  have_mate_keyring=yes], have_mate_keyring=no)
	if test "x$have_mate_keyring" = xno -a "x$enable_mate_keyring" = xyes; then
		AC_MSG_ERROR([mate-keyring support requested but libraries not found])
	fi
fi

AC_SUBST(MATE_KEYRING_LIBS)
AC_SUBST(MATE_KEYRING_CFLAGS)
AM_CONDITIONAL(ENABLE_MATE_KEYRING, [test "$have_mate_keyring" = "yes"])

# *************
# Documentation
# *************

AC_PATH_PROG(SCROLLKEEPER_CONFIG, scrollkeeper-config,no)
if test x$SCROLLKEEPER_CONFIG = xno; then
  AC_MSG_ERROR(Couldn't find scrollkeeper-config, please install the scrollkeeper package)
fi

# ********************
# Internationalization
# ********************

IT_PROG_INTLTOOL([$INTLTOOL_REQUIRED])

GETTEXT_PACKAGE=mate-disk-utility
AC_SUBST([GETTEXT_PACKAGE])
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext domain])

# ******************
# Caja extension
# ******************

have_caja=no
AC_ARG_ENABLE(caja, AS_HELP_STRING([--disable-caja], [disable caja extension]))

if test "x$enable_caja" != "xno"; then
	PKG_CHECK_MODULES(CAJA, libcaja-extension >= $CAJA_REQUIRED,
			  [AC_DEFINE(HAVE_CAJA, 1, [Define if caja is available])
			  have_caja=yes], have_caja=no)
	if test "x$have_caja" = xno -a "x$enable_caja" = xyes; then
		AC_MSG_ERROR([caja support requested but libraries not found])
	fi
fi

AC_SUBST(CAJA_LIBS)
AC_SUBST(CAJA_CFLAGS)
AC_SUBST(CAJA_EXTENSION_DIR)

AM_CONDITIONAL(ENABLE_CAJA, [test "$have_caja" = "yes"])

# *************************************
# *************************************

AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([
Makefile
src/Makefile
src/gdu/Makefile
src/gdu/gdu.pc
src/gdu-gtk/Makefile
src/gdu-gtk/gdu-gtk.pc
src/palimpsest/Makefile
src/notification/Makefile
src/format-tool/Makefile
src/caja-extension/Makefile
po/Makefile.in
data/Makefile
data/gdu-notification-daemon.desktop.in.in
data/icons/Makefile
data/icons/16x16/Makefile
data/icons/22x22/Makefile
data/icons/24x24/Makefile
data/icons/48x48/Makefile
data/icons/scalable/Makefile
help/Makefile
doc/Makefile
doc/version.xml
])

AC_OUTPUT

echo "
                  mate-disk-utility $VERSION
                =============================

        prefix:                     ${prefix}
        libdir:                     ${libdir}
        libexecdir:                 ${libexecdir}
        bindir:                     ${bindir}
        sbindir:                    ${sbindir}
        datadir:                    ${datadir}
        sysconfdir:                 ${sysconfdir}
        localstatedir:              ${localstatedir}

        compiler:                   ${CC}
        cflags:                     ${CFLAGS}
        cppflags:                   ${CPPFLAGS}

        Maintainer mode:            ${USE_MAINTAINER_MODE}
        Building api docs:          ${enable_gtk_doc}
        Caja extension:         ${have_caja}

        MATE Keyring support:      ${have_mate_keyring}
        Remote Access:              ${have_remote_access}
"
