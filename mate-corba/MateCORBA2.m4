dnl AM_PATH_MATECORBA2([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for MateCORBA2, and define MATECORBA_CFLAGS and MATECORBA_LIBS
dnl
AC_DEFUN([AM_PATH_MATECORBA2],
[dnl 
dnl Get the cflags and libraries from the matecorba2-config script
dnl
AC_ARG_WITH(matecorba-prefix,[  --with-matecorba-prefix=PFX   Prefix where MATECORBA is installed (optional)],
            matecorba_config_prefix="$withval", matecorba_config_prefix="")
AC_ARG_WITH(matecorba-exec-prefix,[  --with-matecorba-exec-prefix=PFX Exec prefix where MATECORBA is installed (optional)],
            matecorba_config_exec_prefix="$withval", matecorba_config_exec_prefix="")
AC_ARG_ENABLE(matecorbatest, [  --disable-matecorbatest       Do not try to compile and run a test MATECORBA program],
		    , enable_matecorbatest=yes)

  for module in . $4
  do
      case "$module" in
         server) 
             matecorba_config_args="$matecorba_config_args server"
	 ;;
         client) 
             matecorba_config_args="$matecorba_config_args client"
         ;;
      esac
  done
  if test x$matecorba_config_args = x ; then
	matecorba_config_args=server
  fi

  if test x$matecorba_config_exec_prefix != x ; then
     matecorba_config_args="$matecorba_config_args --exec-prefix=$matecorba_config_exec_prefix"
     if test x${MATECORBA_CONFIG+set} != xset ; then
        MATECORBA_CONFIG=$matecorba_config_exec_prefix/bin/matecorba2-config
     fi
  fi
  if test x$matecorba_config_prefix != x ; then
     matecorba_config_args="$matecorba_config_args --prefix=$matecorba_config_prefix"
     if test x${MATECORBA_CONFIG+set} != xset ; then
        MATECORBA_CONFIG=$matecorba_config_prefix/bin/matecorba2-config
     fi
  fi

  AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  AC_PATH_PROG(MATECORBA_CONFIG, matecorba2-config, no)
  min_matecorba_version=ifelse([$1], , 2.3.0, $1)
  AC_MSG_CHECKING(for MateCORBA - version >= $min_matecorba_version)
  no_matecorba=""
  if test "$MATECORBA_CONFIG" = "no" ; then
    if test "$PKG_CONFIG" = "no" ; then
      no_matecorba=yes
    else
      MATECORBA_CFLAGS=`$PKG_CONFIG --cflags MateCORBA-2.0`
      MATECORBA_LIBS=`$PKG_CONFIG --libs MateCORBA-2.0`
    fi
  else
    MATECORBA_CFLAGS=`$MATECORBA_CONFIG $matecorba_config_args --cflags`
    MATECORBA_LIBS=`$MATECORBA_CONFIG $matecorba_config_args --libs`
  fi

  if test "x$no_matecorba" = x ; then
    MATECORBA_VERSION=`$PKG_CONFIG --modversion MateCORBA-2.0`

    matecorba_config_major_version=`echo $MATECORBA_VERSION | \
	   sed -e 's,[[^0-9.]],,g' -e 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    matecorba_config_minor_version=`echo $MATECORBA_VERSION | \
	   sed -e 's,[[^0-9.]],,g' -e 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    matecorba_config_micro_version=`echo $MATECORBA_VERSION | \
	   sed -e 's,[[^0-9.]],,g' -e 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_matecorbatest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $MATECORBA_CFLAGS"
      LIBS="$MATECORBA_LIBS $LIBS"
dnl
dnl Now check if the installed MATECORBA is sufficiently new. (Also sanity
dnl checks the results of matecorba2-config to some extent
dnl
      rm -f conf.matecorbatest
      AC_TRY_RUN([
#include <matecorba/matecorba.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.matecorbatest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_matecorba_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_matecorba_version");
     exit(1);
   }

  if ((matecorba_major_version != $matecorba_config_major_version) ||
      (matecorba_minor_version != $matecorba_config_minor_version) ||
      (matecorba_micro_version != $matecorba_config_micro_version))
    {
      printf("\n*** 'pkg-config --version MateCORBA-2.0' returned %d.%d.%d, but MateCORBA (%d.%d.%d)\n", 
             $matecorba_config_major_version, $matecorba_config_minor_version, $matecorba_config_micro_version,
             matecorba_major_version, matecorba_minor_version, matecorba_micro_version);
      printf ("*** was found! If matecorba2-config was correct, then it is best\n");
      printf ("*** to remove the old version of MateCORBA. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If matecorba2-config was wrong, set the environment variable MATECORBA_CONFIG\n");
      printf("*** to point to the correct copy of matecorba2-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
#if defined (MATECORBA_MAJOR_VERSION) && defined (MATECORBA_MINOR_VERSION) && defined (MATECORBA_MICRO_VERSION)
  else if ((matecorba_major_version != MATECORBA_MAJOR_VERSION) ||
	   (matecorba_minor_version != MATECORBA_MINOR_VERSION) ||
           (matecorba_micro_version != MATECORBA_MICRO_VERSION))
    {
      printf("*** MateCORBA header files (version %d.%d.%d) do not match\n",
	     MATECORBA_MAJOR_VERSION, MATECORBA_MINOR_VERSION, MATECORBA_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     matecorba_major_version, matecorba_minor_version, matecorba_micro_version);
    }
#endif /* defined (MATECORBA_MAJOR_VERSION) ... */
  else
    {
      if ((matecorba_major_version > major) ||
        ((matecorba_major_version == major) && (matecorba_minor_version > minor)) ||
        ((matecorba_major_version == major) && (matecorba_minor_version == minor) && (matecorba_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of MateCORBA (%d.%d.%d) was found.\n",
               matecorba_major_version, matecorba_minor_version, matecorba_micro_version);
        printf("*** You need a version of MateCORBA newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** MateCORBA is always available from ftp://ftp.matecorba.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the matecorba2-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of MateCORBA, but you can also set the MATECORBA_CONFIG environment to point to the\n");
        printf("*** correct copy of matecorba2-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_matecorba=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_matecorba" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$MATECORBA_CONFIG" = "no" ; then
       echo "*** The matecorba2-config script installed by MATECORBA could not be found"
       echo "*** If MateCORBA was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the MATECORBA_CONFIG environment variable to the"
       echo "*** full path to matecorba2-config."
     else
       if test -f conf.matecorbatest ; then
        :
       else
          echo "*** Could not run MATECORBA test program, checking why..."
          CFLAGS="$CFLAGS $MATECORBA_CFLAGS"
          LIBS="$LIBS $MATECORBA_LIBS"
          AC_TRY_LINK([
#include <matecorba/matecorba.h>
#include <stdio.h>
],      [ return ((matecorba_major_version) || (matecorba_minor_version) || (matecorba_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding MateCORBA or finding the wrong"
          echo "*** version of MateCORBA. If it is not finding MateCORBA, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the MateCORBA package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps matecorba matecorba-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means MATECORBA was incorrectly installed"
          echo "*** or that you have moved MateCORBA since it was installed. In the latter case, you"
          echo "*** may want to edit the matecorba2-config script: $MATECORBA_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     MATECORBA_CFLAGS=""
     MATECORBA_LIBS=""
     ifelse([$3], , :, [$3])
  fi

  AC_PATH_PROG(MATECORBA_IDL, matecorba-idl-2, ifelse([$3], , :, [$3]))
  AC_SUBST(MATECORBA_CFLAGS)
  AC_SUBST(MATECORBA_LIBS)
  AC_SUBST(MATECORBA_IDL)
  rm -f conf.matecorbatest
])
