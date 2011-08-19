#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME=mateconf-editor

REQUIRED_AUTOMAKE_VERSION=1.9
REQUIRED_INTLTOOL_VERSION=0.40.0

if ! test -f $srcdir/src/mateconf-editor-application.c; then
  echo "**Error**: Directory '$srcdir' does not look like the mateconf-edtior source directory"
  exit 1
fi

which mate-autogen.sh || {
    echo "You need to install mate-common from Mate SVN"
    exit 1
}

. mate-autogen.sh
