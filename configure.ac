AC_INIT(src/krb5-auth-dialog.c)

AM_INIT_AUTOMAKE(krb5-auth-dialog, 0.1)
AM_CONFIG_HEADER(config.h)

AM_MAINTAINER_MODE

GETTEXT_PACKAGE=krb5-auth-dialog
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])


AC_PROG_CC
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_LIBTOOL
AC_PROG_INTLTOOL

ALL_LINGUAS=""
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(GNOME,
[
	gtk+-2.0 >= 2.4.0
	libglade-2.0 >= 2.4.0
	libgnomeui-2.0 >= 2.4.0
])

AC_PATH_PROG([KRB5_CONFIG], krb5-config, none, $PATH:/usr/kerberos/bin)
if test "x$KRB5_CONFIG" != "xnone"; then
   KRB5_LIBS="`${KRB5_CONFIG} --libs krb5`"
   KRB5_CFLAGS="`${KRB5_CONFIG} --cflags krb5`"
   AC_SUBST(KRB5_CFLAGS)
   AC_SUBST(KRB5_LIBS)
fi


CFLAGS="$GNOME_CFLAGS $KRB5_CFLAGS $CFLAGS"


AC_OUTPUT([
Makefile
krb5-auth-dialog.spec
src/Makefile
po/Makefile.in
])
