#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

PKG_NAME="gucharmap"

(test -f $srcdir/gucharmap/main.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

which mate-autogen.sh || {
    echo "You need to install mate-common from MATE git and make"
    echo "sure the mate-autogen.sh script is in your \$PATH."
    exit 1
}

REQUIRED_INTLTOOL_VERSION=0.40.4
REQUIRED_AUTOMAKE_VERSION=1.9
REQUIRED_MATE_DOC_UTILS_VERSION=0.9.0
. mate-autogen.sh
