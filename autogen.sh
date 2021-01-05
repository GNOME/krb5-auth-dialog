#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
REQUIRED_AUTOMAKE_VERSION=1.7
REQUIRED_INTLTOOL_VERSION=0.35.0

(test -f $srcdir/configure.ac \
  && test -f $srcdir/src/ka-kerberos.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level krb5-auth-dialog directory"
    exit 1
}


which gnome-autogen.sh || {
    echo "You need to install gnome-common from the GNOME CVS"
    exit 1
}
. gnome-autogen.sh
