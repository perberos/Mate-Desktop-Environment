AC_DEFUN([AM_MATE_CHECK_TYPE],
  [AC_CACHE_CHECK([$1 in <sys/types.h>], ac_cv_type_$1,
     [AC_TRY_COMPILE([
#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
],[$1 foo;],
     ac_cv_type_$1=yes, ac_cv_type_$1=no)])
   if test $ac_cv_type_$1 = no; then
      AC_DEFINE($1, $2, $1)
   fi
])

AC_DEFUN([AM_MATE_SIZE_T],
  [AM_MATE_CHECK_TYPE(size_t, unsigned)
   AC_PROVIDE([AC_TYPE_SIZE_T])
])

AC_DEFUN([AM_MATE_OFF_T],
  [AM_MATE_CHECK_TYPE(off_t, long)
   AC_PROVIDE([AC_TYPE_OFF_T])
])

dnl Autoconf macros for libgnutls

# Modified for LIBGNUTLS -- nmav
# Configure paths for LIBGCRYPT
# Shamelessly stolen from the one of XDELTA by Owen Taylor
# Werner Koch   99-12-09

dnl AM_PATH_LIBGNUTLS([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND ]]])
dnl Test for libgnutls, and define LIBGNUTLS_CFLAGS and LIBGNUTLS_LIBS
dnl
AC_DEFUN([AM_PATH_LIBGNUTLS],
[dnl
dnl Get the cflags and libraries from the libgnutls-config script
dnl
AC_ARG_WITH(libgnutls-prefix,
          [  --with-libgnutls-prefix=PFX   Prefix where libgnutls is installed (optional)],
          libgnutls_config_prefix="$withval", libgnutls_config_prefix="")

  if test x$libgnutls_config_prefix != x ; then
     libgnutls_config_args="$libgnutls_config_args --prefix=$libgnutls_config_prefix"
     if test x${LIBGNUTLS_CONFIG+set} != xset ; then
        LIBGNUTLS_CONFIG=$libgnutls_config_prefix/bin/libgnutls-config
     fi
  fi

  AC_PATH_PROG(LIBGNUTLS_CONFIG, libgnutls-config, no)
  min_libgnutls_version=ifelse([$1], ,0.1.0,$1)
  AC_MSG_CHECKING(for libgnutls - version >= $min_libgnutls_version)
  no_libgnutls=""
  if test "$LIBGNUTLS_CONFIG" = "no" ; then
    no_libgnutls=yes
  else
    LIBGNUTLS_CFLAGS=`$LIBGNUTLS_CONFIG $libgnutls_config_args --cflags`
    LIBGNUTLS_LIBS=`$LIBGNUTLS_CONFIG $libgnutls_config_args --libs`
    libgnutls_config_version=`$LIBGNUTLS_CONFIG $libgnutls_config_args --version`


      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $LIBGNUTLS_CFLAGS"
      LIBS="$LIBS $LIBGNUTLS_LIBS"
dnl
dnl Now check if the installed libgnutls is sufficiently new. Also sanity
dnl checks the results of libgnutls-config to some extent
dnl
      rm -f conf.libgnutlstest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>

int
main ()
{
    system ("touch conf.libgnutlstest");

    if( strcmp( gnutls_check_version(NULL), "$libgnutls_config_version" ) )
    {
      printf("\n*** 'libgnutls-config --version' returned %s, but LIBGNUTLS (%s)\n",
             "$libgnutls_config_version", gnutls_check_version(NULL) );
      printf("*** was found! If libgnutls-config was correct, then it is best\n");
      printf("*** to remove the old version of LIBGNUTLS. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If libgnutls-config was wrong, set the environment variable LIBGNUTLS_CONFIG\n");
      printf("*** to point to the correct copy of libgnutls-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    }
    else if ( strcmp(gnutls_check_version(NULL), LIBGNUTLS_VERSION ) )
    {
      printf("\n*** LIBGNUTLS header file (version %s) does not match\n", LIBGNUTLS_VERSION);
      printf("*** library (version %s)\n", gnutls_check_version(NULL) );
    }
    else
    {
      if ( gnutls_check_version( "$min_libgnutls_version" ) )
      {
        return 0;
      }
     else
      {
        printf("no\n*** An old version of LIBGNUTLS (%s) was found.\n",
                gnutls_check_version(NULL) );
        printf("*** You need a version of LIBGNUTLS newer than %s. The latest version of\n",
               "$min_libgnutls_version" );
        printf("*** LIBGNUTLS is always available from ftp://gnutls.hellug.gr/pub/gnutls.\n");
        printf("*** \n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the libgnutls-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of LIBGNUTLS, but you can also set the LIBGNUTLS_CONFIG environment to point to the\n");
        printf("*** correct copy of libgnutls-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_libgnutls=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
  fi

  if test "x$no_libgnutls" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])
  else
     if test -f conf.libgnutlstest ; then
        :
     else
        AC_MSG_RESULT(no)
     fi
     if test "$LIBGNUTLS_CONFIG" = "no" ; then
       echo "*** The libgnutls-config script installed by LIBGNUTLS could not be found"
       echo "*** If LIBGNUTLS was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the LIBGNUTLS_CONFIG environment variable to the"
       echo "*** full path to libgnutls-config."
     else
       if test -f conf.libgnutlstest ; then
        :
       else
          echo "*** Could not run libgnutls test program, checking why..."
          CFLAGS="$CFLAGS $LIBGNUTLS_CFLAGS"
          LIBS="$LIBS $LIBGNUTLS_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gnutls/gnutls.h>
],      [ return !!gnutls_check_version(NULL); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding LIBGNUTLS or finding the wrong"
          echo "*** version of LIBGNUTLS. If it is not finding LIBGNUTLS, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
          echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means LIBGNUTLS was incorrectly installed"
          echo "*** or that you have moved LIBGNUTLS since it was installed. In the latter case, you"
          echo "*** may want to edit the libgnutls-config script: $LIBGNUTLS_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     LIBGNUTLS_CFLAGS=""
     LIBGNUTLS_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  rm -f conf.libgnutlstest
  AC_SUBST(LIBGNUTLS_CFLAGS)
  AC_SUBST(LIBGNUTLS_LIBS)
])

dnl end of Autoconf macros for libgnutls


dnl macros for the neon bundeled build
# Copyright (C) 1998-2004 Joe Orton <joe@manyfish.co.uk>  
# Modifications by Christian Kellner <gicmo@mate-de.org>
AC_DEFUN([NE_DEFINE_VERSIONS], [

NEON_VERSION="${NE_VERSION_MAJOR}.${NE_VERSION_MINOR}.${NE_VERSION_PATCH}${NE_VERSION_TAG}"

AC_DEFINE_UNQUOTED([NEON_VERSION], ["${NEON_VERSION}"],
                   [Define to be the neon version string])
AC_DEFINE_UNQUOTED([NE_VERSION_MAJOR], [(${NE_VERSION_MAJOR})],
                   [Define to be neon library major version])
AC_DEFINE_UNQUOTED([NE_VERSION_MINOR], [(${NE_VERSION_MINOR})],
                   [Define to be neon library minor version])
AC_DEFINE_UNQUOTED([NE_VERSION_PATCH], [(${NE_VERSION_PATCH})],
                   [Define to be neon library patch version])
])

AC_DEFUN([NE_VERSIONS], [

# Define the current versions.
NE_VERSION_MAJOR=0
NE_VERSION_MINOR=25
NE_VERSION_PATCH=4
NE_VERSION_TAG=

# libtool library interface versioning.  Release policy dictates that
# for neon 0.x.y, each x brings an incompatible interface change, and
# each y brings no interface change, and since this policy has been
# followed since 0.1, x == CURRENT, y == RELEASE, 0 == AGE.  For
# 1.x.y, this will become N + x == CURRENT, y == RELEASE, x == AGE,
# where N is constant (and equal to CURRENT + 1 from the final 0.x
# release)

NEON_INTERFACE_VERSION="${NE_VERSION_MINOR}:${NE_VERSION_PATCH}:0"

NE_DEFINE_VERSIONS

])

AC_DEFUN([VFS_NEON_BUNDLED],[

neon_bundled_srcdir=$1
neon_bundled_builddir=$1

# INLINED This was NEON_COMMON
NE_VERSIONS

AC_MSG_NOTICE([using bundled neon ($NEON_VERSION)])
NEON_BUILD_BUNDLED="yes"
LIBNEON_SOURCE_CHECKS
NEON_CFLAGS="$CFLAGS -I.. -I../.. -I$srcdir/$neon_bundled_srcdir"
NEON_NEED_XML_PARSER=yes
neon_library_message="included libneon (${NEON_VERSION})"

AC_SUBST(NEON_BUILD_BUNDLED)

# INLINED This was NEON_BUILD_LIBTOOL
NEON_TARGET=libneon.la
NEON_OBJEXT=lo

# Using the default set of object files to build.
# Add the extension to EXTRAOBJS
ne="$NEON_EXTRAOBJS"
NEON_EXTRAOBJS=
for o in $ne; do
	NEON_EXTRAOBJS="$NEON_EXTRAOBJS $o.$NEON_OBJEXT"
done	

NEON_SUPPORTS_DAV=yes
NEONOBJS="$NEONOBJS \$(NEON_DAVOBJS)"
# Turn on DAV locking please then.
AC_DEFINE(USE_DAV_LOCKS, 1, [Support WebDAV locking through the library])

AC_SUBST(NEON_TARGET)
AC_SUBST(NEON_OBJEXT)
AC_SUBST(NEONOBJS)
AC_SUBST(NEON_EXTRAOBJS)
AC_SUBST(NEON_LINK_FLAGS)
AC_SUBST(NEON_SUPPORTS_DAV)

#xml parser stuff
PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= $XML_REQUIRED)
NEON_CFLAGS="$NEON_CFLAGS $LIBXML_CFLAGS" 
NEON_LIBS="$NEON_LIBS $LIBXML_LIBS"

AC_DEFINE(HAVE_LIBXML, 1, [Define if you have libxml])

neon_xml_parser=libxml2

])

dnl Checks needed when compiling the neon source.
AC_DEFUN([LIBNEON_SOURCE_CHECKS], [


dnl Run all the normal C language/compiler tests
AC_REQUIRE([NEON_COMMON_CHECKS])

dnl Needed for building the MD5 code.
AC_REQUIRE([AC_C_BIGENDIAN])
dnl Is strerror_r present; if so, which variant
AC_REQUIRE([AC_FUNC_STRERROR_R])

AC_CHECK_HEADERS([strings.h sys/time.h limits.h sys/select.h arpa/inet.h \
	signal.h sys/socket.h netinet/in.h netinet/tcp.h netdb.h])


AC_REPLACE_FUNCS(strcasecmp)

AC_CHECK_FUNCS(signal setvbuf setsockopt stpcpy)

AC_CHECK_MEMBERS(struct tm.tm_gmtoff,,
AC_MSG_WARN([no timezone handling in date parsing on this platform]),
[#include <time.h>])

ifdef([neon_no_zlib], [
    neon_zlib_message="zlib disabled"
    NEON_SUPPORTS_ZLIB=no
], [
    NEON_ZLIB()
])

# Conditionally enable ACL support
AC_MSG_CHECKING([whether to enable ACL support in neon])
if test "x$neon_no_acl" = "xyes"; then
    AC_MSG_RESULT(no)
else
    AC_MSG_RESULT(yes)
    NEON_EXTRAOBJS="$NEON_EXTRAOBJS ne_acl"
fi

NEON_SUPPORTS_SSL=yes
dnl NEON_SSL is an autoconf macro in current version of neon:
m4_pushdef([NEON_SSL], [[NEON_SSL]])
AC_DEFINE([NEON_SSL], 1, [Build neon ssl support])
m4_popdef([NEON_SSL])

if test "x$have_gssapi" = "xyes"; then

	NEON_CFLAGS="$NEON_CFLAGS $GSSAPI_CFLAGS"
	NEON_LIBS="$NEON_LIBS $GSSAPI_LIBS"

fi

AC_SUBST(NEON_CFLAGS)
AC_SUBST(NEON_LIBS)
AC_SUBST(NEON_LTLIBS)

])


#copied and pasted unchanged form the neon macros files
# Copyright (C) 1998-2004 Joe Orton <joe@manyfish.co.uk> 


dnl AC_SEARCH_LIBS done differently. Usage:
dnl   NE_SEARCH_LIBS(function, libnames, [extralibs], [actions-if-not-found],
dnl                            [actions-if-found])
dnl Tries to find 'function' by linking againt `-lLIB $NEON_LIBS' for each
dnl LIB in libnames.  If link fails and 'extralibs' is given, will also
dnl try linking against `-lLIB extralibs $NEON_LIBS`.
dnl Once link succeeds, `-lLIB [extralibs]` is prepended to $NEON_LIBS, and
dnl `actions-if-found' are executed, if given.
dnl If link never succeeds, run `actions-if-not-found', if given, else
dnl give an error and fail configure.
AC_DEFUN([NE_SEARCH_LIBS], [

AC_CACHE_CHECK([for library containing $1], [ne_cv_libsfor_$1], [
AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="none needed"], [
ne_sl_save_LIBS=$LIBS
ne_cv_libsfor_$1="not found"
for lib in $2; do
    LIBS="$ne_sl_save_LIBS -l$lib $NEON_LIBS"
    AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="-l$lib"; break])
    m4_if($3, [], [], dnl If $3 is specified, then...
              [LIBS="$ne_sl_save_LIBS -l$lib $3 $NEON_LIBS"
               AC_TRY_LINK_FUNC($1, [ne_cv_libsfor_$1="-l$lib $3"; break])])
done
LIBS=$ne_sl_save_LIBS])])

if test "$ne_cv_libsfor_$1" = "not found"; then
   m4_if($4, [], [AC_MSG_ERROR([could not find library containing $1])], [$4])
elif test "$ne_cv_libsfor_$1" != "none needed"; then 
   NEON_LIBS="$ne_cv_libsfor_$1 $NEON_LIBS"
   $5
fi])


dnl Check for presence and suitability of zlib library
AC_DEFUN([NEON_ZLIB], [

AC_ARG_WITH(zlib, AC_HELP_STRING([--without-zlib], [disable zlib support]),
ne_use_zlib=$withval, ne_use_zlib=yes)

NEON_SUPPORTS_ZLIB=no
AC_SUBST(NEON_SUPPORTS_ZLIB)

if test "$ne_use_zlib" = "yes"; then
    AC_CHECK_HEADER(zlib.h, [
  	AC_CHECK_LIB(z, inflate, [ 
	    NEON_LIBS="$NEON_LIBS -lz"
	    NEON_CFLAGS="$NEON_CFLAGS -DNEON_ZLIB"
	    NEON_SUPPORTS_ZLIB=yes
	    neon_zlib_message="found in -lz"
	], [neon_zlib_message="zlib not found"])
    ], [neon_zlib_message="zlib not found"])
else
    neon_zlib_message="zlib disabled"
fi
])

AC_DEFUN([NE_MACOSX], [
# Check for Darwin, which needs extra cpp and linker flags.
AC_CACHE_CHECK([for Darwin], ne_cv_os_macosx, [
case `uname -s 2>/dev/null` in
Darwin) ne_cv_os_macosx=yes ;;
*) ne_cv_os_macosx=no ;;
esac])
if test $ne_cv_os_macosx = yes; then
  CPPFLAGS="$CPPFLAGS -no-cpp-precomp"
  LDFLAGS="$LDFLAGS -flat_namespace"
fi
])

AC_DEFUN([NEON_FORMAT_PREP], [
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(long long)
if test "$GCC" = "yes"; then
  AC_CACHE_CHECK([for gcc -Wformat -Werror sanity], ne_cv_cc_werror, [
  # See whether a simple test program will compile without errors.
  ne_save_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS -Wformat -Werror"
  AC_TRY_COMPILE([#include <sys/types.h>
  #include <stdio.h>], [int i = 42; printf("%d", i);], 
  [ne_cv_cc_werror=yes], [ne_cv_cc_werror=no])
  CPPFLAGS=$ne_save_CPPFLAGS])
  ne_fmt_trycompile=$ne_cv_cc_werror
else
  ne_fmt_trycompile=no
fi
])

dnl NEON_FORMAT(TYPE[, HEADERS[, [SPECIFIER]])
dnl
dnl This macro finds out which modifier is needed to create a
dnl printf format string suitable for printing integer type TYPE (which
dnl may be an int, long, or long long).
dnl The default specifier is 'd', if SPECIFIER is not given.  
dnl TYPE may be defined in HEADERS; sys/types.h is always used first.
AC_DEFUN([NEON_FORMAT], [

AC_REQUIRE([NEON_FORMAT_PREP])

AC_CHECK_SIZEOF($1, [$2])

dnl Work out which specifier character to use
m4_ifdef([ne_spec], [m4_undefine([ne_spec])])
m4_if($#, 3, [m4_define(ne_spec,$3)], [m4_define(ne_spec,d)])

AC_CACHE_CHECK([how to print $1], [ne_cv_fmt_$1], [
ne_cv_fmt_$1=none
if test $ne_fmt_trycompile = yes; then
  oflags="$CPPFLAGS"
  # Consider format string mismatches as errors
  CPPFLAGS="$CPPFLAGS -Wformat -Werror"
  dnl obscured for m4 quoting: "for str in d ld qd; do"
  for str in ne_spec l]ne_spec[ q]ne_spec[; do
    AC_TRY_COMPILE([#include <sys/types.h>
$2
#include <stdio.h>], [$1 i = 1; printf("%$str", i);], 
	[ne_cv_fmt_$1=$str; break])
  done
  CPPFLAGS=$oflags
else
  # Best guess. Don't have to be too precise since we probably won't
  # get a warning message anyway.
  case $ac_cv_sizeof_$1 in
  $ac_cv_sizeof_int) ne_cv_fmt_$1="ne_spec" ;;
  $ac_cv_sizeof_long) ne_cv_fmt_$1="l]ne_spec[" ;;
  $ac_cv_sizeof_long_long) ne_cv_fmt_$1="ll]ne_spec[" ;;
  esac
fi
])

if test "x$ne_cv_fmt_$1" = "xnone"; then
  AC_MSG_ERROR([format string for $1 not found])
fi

AC_DEFINE_UNQUOTED([NE_FMT_]translit($1, a-z, A-Z), "$ne_cv_fmt_$1", 
	[Define to be printf format string for $1])
])

dnl Wrapper for AC_CHECK_FUNCS; uses libraries from $NEON_LIBS.
AC_DEFUN([NE_CHECK_FUNCS], [
ne_cf_save_LIBS=$LIBS
LIBS="$LIBS $NEON_LIBS"
AC_CHECK_FUNCS($@)
LIBS=$ne_cf_save_LIBS])


dnl Find 'ar' and 'ranlib', fail if ar isn't found.
AC_DEFUN([NE_FIND_AR], [

# Search in /usr/ccs/bin for Solaris
ne_PATH=$PATH:/usr/ccs/bin
AC_PATH_TOOL(AR, ar, notfound, $ne_PATH)
if test "x$AR" = "xnotfound"; then
   AC_MSG_ERROR([could not find ar tool])
fi
AC_PATH_TOOL(RANLIB, ranlib, :, $ne_PATH)

])

dnl Less noisy replacement for PKG_CHECK_MODULES
AC_DEFUN([NE_PKG_CONFIG], [

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "$PKG_CONFIG" = "no"; then
   : Not using pkg-config
   $4
else
   AC_CACHE_CHECK([for $2 pkg-config data], ne_cv_pkg_$2,
     [if $PKG_CONFIG $2; then
        ne_cv_pkg_$2=yes
      else
        ne_cv_pkg_$2=no
      fi])

   if test "$ne_cv_pkg_$2" = "yes"; then
      $1_CFLAGS=`$PKG_CONFIG --cflags $2`
      $1_LIBS=`$PKG_CONFIG --libs $2`
      : Using provided pkg-config data
      $3
   else
      : No pkg-config for $2 provided
      $4
   fi
fi])


AC_DEFUN([NEON_COMMON_CHECKS], [

# These checks are done whether or not the bundled neon build
# is used.

AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_PROG_CC_STDC])
AC_REQUIRE([AC_LANG_C])
AC_REQUIRE([AC_ISC_POSIX])
AC_REQUIRE([AC_C_INLINE])
AC_REQUIRE([AC_C_CONST])
AC_REQUIRE([AC_TYPE_SIZE_T])
AC_REQUIRE([AC_TYPE_OFF_T])

AC_REQUIRE([NE_MACOSX])

AC_REQUIRE([AC_PROG_MAKE_SET])

AC_REQUIRE([AC_HEADER_STDC])

AC_CHECK_HEADERS([errno.h stdarg.h string.h stdlib.h])

NEON_FORMAT(size_t,,u) dnl size_t is unsigned; use %u formats
NEON_FORMAT(off_t)
NEON_FORMAT(ssize_t)

])


dnl end of neon macros
