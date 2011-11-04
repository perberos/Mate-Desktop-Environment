dnl as-ac-expand.m4 0.2.0
dnl autostars m4 macro for expanding directories using configure's prefix
dnl thomas@apestaart.org

dnl AS_AC_EXPAND(VAR, CONFIGURE_VAR)
dnl example
dnl AS_AC_EXPAND(SYSCONFDIR, $sysconfdir)
dnl will set SYSCONFDIR to /usr/local/etc if prefix=/usr/local

AC_DEFUN([AS_AC_EXPAND],
[
  EXP_VAR=[$1]
  FROM_VAR=[$2]

  dnl first expand prefix and exec_prefix if necessary
  prefix_save=$prefix
  exec_prefix_save=$exec_prefix

  dnl if no prefix given, then use /usr/local, the default prefix
  if test "x$prefix" = "xNONE"; then
    prefix="$ac_default_prefix"
  fi
  dnl if no exec_prefix given, then use prefix
  if test "x$exec_prefix" = "xNONE"; then
    exec_prefix=$prefix
  fi

  full_var="$FROM_VAR"
  dnl loop until it doesn't change anymore
  while true; do
    new_full_var="`eval echo $full_var`"
    if test "x$new_full_var" = "x$full_var"; then break; fi
    full_var=$new_full_var
  done

  dnl clean up
  full_var=$new_full_var
  AC_SUBST([$1], "$full_var")

  dnl restore prefix and exec_prefix
  prefix=$prefix_save
  exec_prefix=$exec_prefix_save
])

dnl Checks for availability of various utmp fields
dnl
dnl Original code by Bernhard Rosenkraenzer (bero@linux.net.eu.org), 1998.
dnl Modifications by Timur Bakeyev (timur@gnu.org), 1999.
dnl Patched from http://bugzilla.gnome.org/show_bug.cgi?id=937
dnl

dnl MDM_CHECK_UTMP()
dnl Test for presence of the field and define HAVE_UT_UT_field macro
dnl Taken from vte/mate-pty-helper's GPH_CHECK_UTMP

AC_DEFUN([MDM_CHECK_UTMP],[

AC_CHECK_HEADERS(sys/time.h utmp.h utmpx.h)
AC_HEADER_TIME

if test "$ac_cv_header_utmpx_h" = "yes"; then
    AC_DEFINE(UTMP,[struct utmpx],[Define to the name of a structure which holds utmp data.])
else
    AC_DEFINE(UTMP,[struct utmp],[Define to the name of a structure which holds utmp data.])
fi

dnl some systems (BSD4.4-like) require time.h to be included before utmp.h :/
AC_MSG_CHECKING(for ut_host field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; char *p; p=ut.ut_host;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_HOST,1,[Define if your utmp struct contains a ut_host field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_pid field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; int i; i=ut.ut_pid;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_PID,1,[Define if your utmp struct contains a ut_pid field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_id field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; char *p; p=ut.ut_id;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_ID,1,[Define if your utmp struct contains a ut_id field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_name field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; char *p; p=ut.ut_name;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_NAME,1,[Define if your utmp struct contains a ut_name field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_type field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; int i; i=(int) ut.ut_type;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_TYPE,1,[Define if your utmp struct contains a ut_type field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_exit.e_termination field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; ut.ut_exit.e_termination=0;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_EXIT_E_TERMINATION,1,[Define if your utmp struct contains a ut_exit.e_termination field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_user field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; char *p; p=ut.ut_user;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_USER,1,[Define if your utmp struct contains a ut_user field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_time field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; ut.ut_time=0;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_TIME,1,[Define if your utmp struct contains a ut_time field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_tv field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; ut.ut_tv.tv_sec=0; ut.ut_tv.tv_usec=0; ],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_TV,1,[Define if your utmp struct contains a ut_tv field.])
fi
AC_MSG_RESULT($result)

AC_MSG_CHECKING(for ut_syslen field in the utmp structure)
AC_TRY_COMPILE([#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif
#ifdef HAVE_UTMP_H
#include <utmp.h>
#endif
#ifdef HAVE_UTMPX_H
#include <utmpx.h>
#endif],[UTMP ut; ut.ut_syslen=0;],result=yes,result=no)
if test "$result" = "yes"; then
  AC_DEFINE(HAVE_UT_UT_SYSLEN,1,[Define if your utmp struct contains a ut_syslen field.])
fi
AC_MSG_RESULT($result)

])

dnl EXTRA_COMPILE_WARNINGS
dnl Turn on many useful compiler warnings
dnl For now, only works on GCC
AC_DEFUN([EXTRA_COMPILE_WARNINGS],[
    dnl ******************************
    dnl More compiler warnings
    dnl ******************************

    AC_ARG_ENABLE(compile-warnings,
                  AC_HELP_STRING([--enable-compile-warnings=@<:@no/minimum/yes/maximum/error@:>@],
                                 [Turn on compiler warnings]),,
                  [enable_compile_warnings="m4_default([$1],[yes])"])

    warnCFLAGS=
    if test "x$GCC" != xyes; then
	enable_compile_warnings=no
    fi

    warning_flags=
    realsave_CFLAGS="$CFLAGS"

    case "$enable_compile_warnings" in
    no)
	warning_flags=
	;;
    minimum)
	warning_flags="-Wall"
	;;
    yes)
	warning_flags="-Wall -Wmissing-prototypes"
	;;
    maximum|error)
	warning_flags="-Wall -Wmissing-prototypes -Wnested-externs -Wpointer-arith"
	CFLAGS="$warning_flags $CFLAGS"
	for option in -Wno-sign-compare; do
		SAVE_CFLAGS="$CFLAGS"
		CFLAGS="$CFLAGS $option"
		AC_MSG_CHECKING([whether gcc understands $option])
		AC_TRY_COMPILE([], [],
			has_option=yes,
			has_option=no,)
		CFLAGS="$SAVE_CFLAGS"
		AC_MSG_RESULT($has_option)
		if test $has_option = yes; then
		  warning_flags="$warning_flags $option"
		fi
		unset has_option
		unset SAVE_CFLAGS
	done
	unset option
	if test "$enable_compile_warnings" = "error" ; then
	    warning_flags="$warning_flags -Werror"
	fi
	;;
    *)
	AC_MSG_ERROR(Unknown argument '$enable_compile_warnings' to --enable-compile-warnings)
	;;
    esac
    CFLAGS="$realsave_CFLAGS"
    AC_MSG_CHECKING(what warning flags to pass to the C compiler)
    AC_MSG_RESULT($warning_flags)

    AC_ARG_ENABLE(iso-c,
                  AC_HELP_STRING([--enable-iso-c],
                                 [Try to warn if code is not ISO C ]),,
                  [enable_iso_c=no])

    AC_MSG_CHECKING(what language compliance flags to pass to the C compiler)
    complCFLAGS=
    if test "x$enable_iso_c" != "xno"; then
	if test "x$GCC" = "xyes"; then
	case " $CFLAGS " in
	    *[\ \	]-ansi[\ \	]*) ;;
	    *) complCFLAGS="$complCFLAGS -ansi" ;;
	esac
	case " $CFLAGS " in
	    *[\ \	]-pedantic[\ \	]*) ;;
	    *) complCFLAGS="$complCFLAGS -pedantic" ;;
	esac
	fi
    fi
    AC_MSG_RESULT($complCFLAGS)

    WARN_CFLAGS="$warning_flags $complCFLAGS"
    AC_SUBST(WARN_CFLAGS)
])
