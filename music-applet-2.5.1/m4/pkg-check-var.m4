dnl PKG_CHECK_VAR(TARGET, VARNAME, LIBS, [ACTION-IF], [ACTION-NOT])
dnl defines TARGET to be the output of running
dnl   "pkg-config --variable=VARNAME LIBS"
dnl also defines TARGET_PKG_ERRORS on error
dnl
dnl Based on PKG_CHECK_MODULES provided by pkg-config.
AC_DEFUN([PKG_CHECK_VAR], [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  if test "$PKG_CONFIG" = "no" ; then
    echo "*** The pkg-config script could not be found. Make sure it is"
    echo "*** in your path, or set the PKG_CONFIG environment variable"
    echo "*** to the full path to pkg-config."
    echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get pkg-config."
  else
    MIN_VERSION=0.9.0
    if $PKG_CONFIG --atleast-pkgconfig-version $MIN_VERSION; then
      AC_MSG_CHECKING(for $3)

      if $PKG_CONFIG --exists "$3" ; then
        AC_MSG_RESULT(yes)
        succeeded=yes

        AC_MSG_CHECKING($1)
        $1=`$PKG_CONFIG --variable=$2 "$3"`
        AC_MSG_RESULT($$1)
      else
        $1=""
        ## If we have a custom action on failure, don't print errors, but
        ## do set a variable so people can do so.
        $1_PKG_ERRORS=`$PKG_CONFIG --errors-to-stdout --print-errors "$3"`
        ifelse([$5], ,echo $$1_PKG_ERRORS,)
      fi

      AC_SUBST($1)
    else
      echo "*** Your version of pkg-config is too old. You need version $MIN_VERSION or newer."
      echo "*** See http://www.freedesktop.org/software/pkgconfig"
    fi
  fi

  if test $succeeded = yes; then
    ifelse([$4], , :, [$4])
  else
    ifelse([$5], , AC_MSG_ERROR([Library requirements ($3) not met; consider adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a nonstandard prefix so pkg-config can find them.]), [$5])
  fi
])
