-*- mode: autoconf -*-
AC_PREREQ(2.52)

dnl the mate-python version number
m4_define(mate_python_major_version, 2)
m4_define(mate_python_minor_version, 28)
m4_define(mate_python_micro_version, 1)
m4_define(mate_python_version, dnl
  mate_python_major_version.mate_python_minor_version.mate_python_micro_version)

dnl required versions of other packages
m4_define(pygtk_required_major_version, 2)
m4_define(pygtk_required_minor_version, 10)
m4_define(pygtk_required_micro_version, 3)
m4_define(pygtk_required_version, pygtk_required_major_version.pygtk_required_minor_version.pygtk_required_micro_version)
m4_define(pymatecorba_required_version,           2.0.1)
m4_define(glib_required_version,              2.6.0)
m4_define(gtk_required_version,               2.6.0)
m4_define(libmate_required_version,          2.8.0)
m4_define(libmateui_required_version,        2.8.0)
m4_define(libmatecanvas_required_version,    2.8.0)
m4_define(libmatevfs_required_version,       2.14.0)
m4_define(mateconf_required_version,             2.11.1)
m4_define(matecomponent_activation_required_version, 2.8.0)
m4_define(libmatecomponent_required_version,         2.8.0)
m4_define(libmatecomponentui_required_version,       2.8.0)
m4_define(pygobject_required_version,         2.17.0)

AC_INIT(mate-python, mate_python_version,
        [http://bugzilla.mate.org/enter_bug.cgi?product=mate-python])
dnl AC_CONFIG_SRCDIR([mate/uimodule.c])
AM_CONFIG_HEADER(config.h)

AM_INIT_AUTOMAKE

AC_DEFINE(MATE_PYTHON_MAJOR_VERSION, mate_python_major_version, [Mate-Python major version])
AC_DEFINE(MATE_PYTHON_MINOR_VERSION, mate_python_minor_version, [Mate-Python minor version])
AC_DEFINE(MATE_PYTHON_MICRO_VERSION, mate_python_micro_version, [Mate-Python macro version])

dnl Define pygtk required version, for runtime check
AC_DEFINE(PYGTK_REQUIRED_MAJOR_VERSION, pygtk_required_major_version, [PyGTK required major version])
AC_DEFINE(PYGTK_REQUIRED_MINOR_VERSION, pygtk_required_minor_version, [PyGTK required minor version])
AC_DEFINE(PYGTK_REQUIRED_MICRO_VERSION, pygtk_required_micro_version, [PyGTK required micro version])

dnl AC_ARG_BINDING(NAME, DEFAULT [, MODULENAME])
AC_DEFUN([AC_ARG_BINDING],
[AC_ARG_ENABLE([$1],
             AC_HELP_STRING([--ifelse([$2], [YES], dis, en)able-[$1]],
                            [build ifelse([$3], , [$1], [$3]) module (default is [$2])]),
             [if test "$enable_[$1]" = "yes"; then 
	     		build_[$1]=true
			  else
				build_[$1]=false
			 fi])
])

MATE_COMMON_INIT

dnl put the ACLOCAL flags in the makefile
ACLOCAL="$ACLOCAL $ACLOCAL_FLAGS"

AC_DISABLE_STATIC
AC_PROG_LIBTOOL
dnl when using libtool 2.x create libtool early, because it's used in configure
m4_ifdef([LT_OUTPUT], [LT_OUTPUT])

dnl check for python
AM_PATH_PYTHON(2.2)
AM_CHECK_PYTHON_HEADERS(,[AC_MSG_ERROR(could not find Python headers or library)])
dnl make the python compiler used globally
CC="$PYTHON_CC"

dnl check for pygobject
PKG_CHECK_MODULES(PYGOBJECT, pygobject-2.0 >= pygobject_required_version)

dnl check for pygtk
PKG_CHECK_MODULES(PYGTK, pygtk-2.0 >= pygtk_required_version)
AC_SUBST(PYGTK_CFLAGS)

dnl check for codegen script
AC_PATH_PROG(PYGOBJECT_CODEGEN, pygobject-codegen-2.0, no)
if test "x$PYGOBJECT_CODEGEN" = xno; then
  dnl This is for compat with older releases when codegen was shipped
  dnl in pygtk. It should be removed in future releases.
  AC_PATH_PROG(PYGTK_CODEGEN, pygtk-codegen-2.0, no)
  if test "x$PYGTK_CODEGEN" = xno; then
    AC_MSG_ERROR(could not find pygobject-codegen-2.0 script)
  else
    AC_SUBST(PYMATE_CODEGEN, $PYGTK_CODEGEN)
  fi
else
  AC_SUBST(PYMATE_CODEGEN, $PYGOBJECT_CODEGEN)
fi

AC_MSG_CHECKING([for conflicting pygtk versions])
pygtk_version=`$PKG_CONFIG --modversion pygtk-2.0`
case $pygtk_version in
     2.9.0|2.9.1)
        AC_MSG_RESULT([found $pygtk_version])
        AC_MSG_ERROR([invalid pygtk version found; please upgrade])
        ;;
     *)
        AC_MSG_RESULT([none])
        ;;
esac

AC_MSG_CHECKING(for pygtk defs)
PYGTK_DEFSDIR=`$PKG_CONFIG --variable=defsdir pygtk-2.0`
AC_SUBST(PYGTK_DEFSDIR)
AC_MSG_RESULT($PYGTK_DEFSDIR)

dnl required version substitutions for use in the spec file
PYGTK_VERSION=pygtk_required_version
LIBMATECANVAS_VERSION=libmatecanvas_required_version
LIBMATE_VERSION=libmate_required_version
LIBMATEVFS_VERSION=libmatevfs_required_version
GTK_VERSION=gtk_required_version
PYMATECORBA_VERSION=pymatecorba_required_version
MATECOMPONENT_ACTIVATION_VERSION=matecomponent_activation_required_version
LIBMATECOMPONENT_VERSION=libmatecomponent_required_version
LIBMATECOMPONENTUI_VERSION=libmatecomponentui_required_version
MATECONF_VERSION=mateconf_required_version
AC_SUBST(PYGTK_VERSION)
AC_SUBST(LIBMATECANVAS_VERSION)
AC_SUBST(LIBMATE_VERSION)
AC_SUBST(LIBMATEVFS_VERSION)
AC_SUBST(GLIB_VERSION)
AC_SUBST(GTK_VERSION)
AC_SUBST(PYMATECORBA_VERSION)
AC_SUBST(MATECOMPONENT_ACTIVATION_VERSION)
AC_SUBST(LIBMATECOMPONENT_VERSION)
AC_SUBST(LIBMATECOMPONENTUI_VERSION)
AC_SUBST(MATECONF_VERSION)

dnl get rid of the -export-dynamic stuff from the configure flags ...
export_dynamic=`(./libtool --config; echo eval echo \\$export_dynamic_flag_spec) | sh`

dnl check for glib
AM_PATH_GLIB_2_0(glib_required_version,,[AC_MSG_ERROR(maybe you want the gtk-mate-1-2 branch?)],$extra_mods)
if test -n "$export_dynamic"; then
  GLIB_LIBS=`echo $GLIB_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl check to make sure we can find gtk
AM_PATH_GTK_2_0(gtk_required_version,,[AC_MSG_ERROR(maybe you want the gtk-mate-1-2 branch?)],$extra_mods)
if test -n "$export_dynamic"; then
  GTK_LIBS=`echo $GTK_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl ****************************************************************************
dnl * Check for prctl() or setproctitle()
dnl ****************************************************************************
AC_CHECK_FUNCS(prctl setproctitle)

AC_ARG_ENABLE([gtk-doc],
             AC_HELP_STRING([--disable-gtk-doc],
                            [Disables gtk-doc documentation generation]),,
		[enable_gtk_doc="yes"])
AM_CONDITIONAL(BUILD_DOCS, [test "$enable_gtk_doc" = "yes"])

dnl default bindings to build:
default_bindings_on="mate mateui matecanvas matevfs matevfsmatecomponent pyvfsmodule mateconf matecomponent_activation matecomponent matecomponentui"
default_bindings_off=""
default_bindings_all="$default_bindings_on $default_bindings_off"

AC_ARG_ENABLE([allbindings],
             AC_HELP_STRING([--disable-allbindings],
                            [Changes all the bindings default values to be either enabled or disabled, overridden by any explicit bindings on the commandline.]),
             [if test "$enable_allbindings" = "no"; then
			   default_bindings_off="$default_bindings_all"
			   default_bindings_on=""
		  	 else
			   default_bindings_off=""
			   default_bindings_on="$default_bindings_all"
		     fi])
for binding in $default_bindings_on; do
	eval build_$binding=true
done
for binding in $default_bindings_off; do
	eval build_$binding=false
done


have_pymatecorba=false
PKG_CHECK_MODULES(PYMATECORBA,
  pymatecorba-2 >= pymatecorba_required_version,
  have_pymatecorba=true, have_pymatecorba=false)
AC_SUBST(PYMATECORBA_CFLAGS)

AC_ARG_BINDING([mate], [ON])
if $build_mate; then
  PKG_CHECK_MODULES(MATE, libmate-2.0 >= libmate_required_version,,
    [AC_MSG_ERROR(libmate-2.0 is required.)])
  AC_SUBST(MATE_CFLAGS)
  AC_SUBST(MATE_LIBS)
  if test -n "$export_dynamic"; then
    MATE_LIBS=`echo $MATE_LIBS | sed -e "s/$export_dynamic//"`
  fi
fi
AM_CONDITIONAL(BUILD_MATE, $build_mate)

AC_ARG_BINDING([mateui], [ON], [mate.ui])
if $build_mateui; then
  PKG_CHECK_MODULES(MATEUI, libmateui-2.0 >= libmateui_required_version,
    build_mateui=true,
    build_mateui=false)
  AC_SUBST(MATEUI_CFLAGS)
  AC_SUBST(MATEUI_LIBS)
fi
AM_CONDITIONAL(BUILD_MATEUI, $build_mate && $have_pymatecorba && $build_mateui)
if test -n "$export_dynamic"; then
  MATEUI_LIBS=`echo $MATEUI_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the mate.canvas module?
AC_ARG_BINDING([matecanvas], [ON])
if $build_matecanvas; then
  PKG_CHECK_MODULES(MATECANVAS, libmatecanvas-2.0 >= libmatecanvas_required_version,
    build_matecanvas=true,
    build_matecanvas=false)
  AC_SUBST(MATECANVAS_CFLAGS)
  AC_SUBST(MATECANVAS_LIBS)
fi
AM_CONDITIONAL(BUILD_MATECANVAS, $build_matecanvas)
if test -n "$export_dynamic"; then
  MATECANVAS_LIBS=`echo $MATECANVAS_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the matevfs module?
AC_ARG_BINDING([matevfs], [ON])
if $build_matevfs; then
  PKG_CHECK_MODULES(MATEVFS, [mate-vfs-2.0 >= libmatevfs_required_version],
    build_matevfs=true,
    build_matevfs=false)
  AC_SUBST(MATEVFS_CFLAGS)
  AC_SUBST(MATEVFS_LIBS)
fi
AM_CONDITIONAL(BUILD_MATEVFS, $build_matevfs)
if test -n "$export_dynamic"; then
  MATEVFS_LIBS=`echo $MATEVFS_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build matecomponent support into the matevfs module?
AC_ARG_BINDING([matevfsmatecomponent], [ON], [matevfs.matecomponent])
if $build_matevfsmatecomponent; then
  PKG_CHECK_MODULES(MATEVFSMATECOMPONENT, [mate-vfs-2.0 >= libmatevfs_required_version dnl
				     matecomponent-activation-2.0 >= matecomponent_activation_required_version
				     pymatecorba-2 >= pymatecorba_required_version],
    build_matevfsmatecomponent=true,
    build_matevfsmatecomponent=false)
  AC_SUBST(MATEVFSMATECOMPONENT_CFLAGS)
  AC_SUBST(MATEVFSMATECOMPONENT_LIBS)
fi
AM_CONDITIONAL(BUILD_MATEVFSMATECOMPONENT, $build_matevfs && $build_matevfsmatecomponent)
if test -n "$export_dynamic"; then
  MATEVFSMATECOMPONENT_LIBS=`echo $MATEVFSMATECOMPONENT_LIBS | sed -e "s/$export_dynamic//"`
fi


dnl should we build the matevfs python module bridge?
AC_ARG_BINDING([pyvfsmodule], [ON], [bridge])
if $build_pyvfsmodule; then
  PKG_CHECK_MODULES(MATE_VFS_MODULE, [mate-vfs-module-2.0 >= libmatevfs_required_version dnl
				       matecomponent-activation-2.0 >= matecomponent_activation_required_version],
    build_pyvfsmodule=true,
    build_pyvfsmodule=false)
fi
AM_CONDITIONAL(BUILD_PYVFSMODULE, $build_matevfs && $build_pyvfsmodule)
if test -n "$export_dynamic"; then
  MATE_VFS_MODULE_LIBS=`echo $MATE_VFS_MODULE_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the mateconf module?
AC_ARG_BINDING([mateconf], [ON])
if $build_mateconf; then
  PKG_CHECK_MODULES(MATECONF, mateconf-2.0 >= mateconf_required_version,
    build_mateconf=true,
    build_mateconf=false)
  MATECONF_INCLUDEDIR="`pkg-config mateconf-2.0 --variable includedir`/mateconf/2/mateconf"
  AC_SUBST(MATECONF_INCLUDEDIR)
  AC_SUBST(MATECONF_CFLAGS)
  AC_SUBST(MATECONF_LIBS)
fi
AM_CONDITIONAL(BUILD_MATECONF, $build_mateconf)
if test -n "$export_dynamic"; then
  MATECONF_LIBS=`echo $MATECONF_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the matecomponent.activation module?
AC_ARG_BINDING([matecomponent_activation], [ON], [matecomponent.activation])
if $build_matecomponent_activation; then
  PKG_CHECK_MODULES(MATECOMPONENT_ACTIVATION, matecomponent-activation-2.0 >= matecomponent_activation_required_version,
    build_matecomponent_activation=true, build_matecomponent_activation=false)
  AC_SUBST(MATECOMPONENT_ACTIVATION_CFLAGS)
  AC_SUBST(MATECOMPONENT_ACTIVATION_LIBS)
fi
AM_CONDITIONAL(BUILD_MATECOMPONENT_ACTIVATION, $have_pymatecorba && $build_matecomponent_activation)
if test -n "$export_dynamic"; then
  MATECOMPONENT_ACTIVATION_LIBS=`echo $MATECOMPONENT_ACTIVATION_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the matecomponent._matecomponent module?
AC_ARG_BINDING([matecomponent], [ON], [matecomponent._matecomponent])
if $build_matecomponent; then
  PKG_CHECK_MODULES(MATECOMPONENT, libmatecomponent-2.0 >= libmatecomponent_required_version,
    build_matecomponent=true, build_matecomponent=false)
  AC_SUBST(MATECOMPONENT_CFLAGS)
  AC_SUBST(MATECOMPONENT_LIBS)
fi
AM_CONDITIONAL(BUILD_MATECOMPONENT, $have_pymatecorba && $build_matecomponent)
if test -n "$export_dynamic"; then
  MATECOMPONENT_LIBS=`echo $MATECOMPONENT_LIBS | sed -e "s/$export_dynamic//"`
fi

dnl should we build the matecomponent.ui module?
AC_ARG_BINDING([matecomponentui], [ON], [matecomponent.ui])
if $build_matecomponentui; then
  PKG_CHECK_MODULES(MATECOMPONENTUI, libmatecomponentui-2.0 >= libmatecomponentui_required_version libmate-2.0 >= libmate_required_version,
    build_matecomponentui=true, build_matecomponentui=false)
  AC_SUBST(MATECOMPONENTUI_CFLAGS)
  AC_SUBST(MATECOMPONENTUI_LIBS)
fi
AM_CONDITIONAL(BUILD_MATECOMPONENTUI, $have_pymatecorba && $build_matecomponentui)
if test -n "$export_dynamic"; then
  MATECOMPONENTUI_LIBS=`echo $MATECOMPONENTUI_LIBS | sed -e "s/$export_dynamic//"`
fi

AM_CONDITIONAL(BUILD_MATECOMPONENT_ANY, $build_matecomponent || $build_matecomponentui || $build_matecomponent_activation)


dnl add required cflags ...
JH_ADD_CFLAG([-Wall])
JH_ADD_CFLAG([-std=c9x])
JH_ADD_CFLAG([-fno-strict-aliasing])

AC_CONFIG_FILES(
  Makefile
  docs/Makefile
  docs/matevfs/Makefile
  mateconf/Makefile
  mate/Makefile
  matevfs/Makefile
  matecanvas/Makefile
  matecomponent/Makefile
  tests/Makefile
  mate-python.spec
  mate-python-2.0.pc
  PKG-INFO)
AC_OUTPUT

echo
echo "The following modules will be built:"
echo
$build_mate             && echo     mate
$build_mateui           && $have_pymatecorba && echo     mate.ui
$build_matecanvas       && echo     matecanvas
$build_matevfs          && echo     matevfs
$build_matevfsmatecomponent    && echo     matevfs.matecomponent
$build_pyvfsmodule       && echo     mate-vfs python module support
$build_mateconf             && echo     mateconf
$build_matecomponent            && $have_pymatecorba && echo     matecomponent
$build_matecomponent_activation && $have_pymatecorba && echo     matecomponent.activation
$build_matecomponentui          && $have_pymatecorba && echo     matecomponent.ui
echo

echo "The following modules will NOT be built:"
echo
$build_mate             || echo     mate
$build_mateui           && $have_pymatecorba || echo     mate.ui
$build_matecanvas       || echo     matecanvas
$build_matevfs          || echo     matevfs
$build_matevfsmatecomponent    || echo     matevfs.matecomponent
$build_pyvfsmodule       || echo     mate-vfs python module support
$build_mateconf             || echo     mateconf
$build_matecomponent            && $have_pymatecorba || echo     matecomponent
$build_matecomponent_activation && $have_pymatecorba || echo     matecomponent.activation
$build_matecomponentui          && $have_pymatecorba || echo     matecomponent.ui

if $have_pymatecorba; then
  :
else
  echo
  echo "Note: PyMateCORBA pymatecorba_required_version or later is required by matecomponent"
  echo "and mateui bindings."
  echo "Download it from: http://ftp.mate.org/pub/MATE/sources/pymatecorba/2.0/"
fi

echo "*****************************************************************************"
echo "*** WARNING: You are using a deprecated build system (autotools)."
echo "***          You should be using WAF instead; see the file INSTALL.WAF."
echo "***          The autotools build system will be removed in a future version."
echo "*****************************************************************************"
