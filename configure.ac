AC_INIT(src/krb5-auth-dialog.c)

AM_INIT_AUTOMAKE(krb5-auth-dialog, 0.2)
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
AM_PROG_LEX
AC_PROG_YACC

ALL_LINGUAS="nb no"
AM_GLIB_GNU_GETTEXT

PKG_CHECK_MODULES(GNOME,
[
	gtk+-2.0 >= 2.4.0
	libglade-2.0 >= 2.4.0
	libgnomeui-2.0 >= 2.4.0
])

AC_ARG_WITH(heimdal, [ --with-heimdal ], with_heimdal="$withval", with_heimdal="no" )
if test "x$with_heimdal" != "xno"; then
	AC_DEFINE_UNQUOTED(HEIMDAL, "", "")
fi

AC_PATH_PROG([KRB5_CONFIG], krb5-config, none, $PATH:/usr/kerberos/bin)
if test "x$KRB5_CONFIG" != "xnone"; then
   KRB5_LIBS="`${KRB5_CONFIG} --libs krb5`"
   KRB5_CFLAGS="`${KRB5_CONFIG} --cflags krb5`"
   AC_SUBST(KRB5_CFLAGS)
   AC_SUBST(KRB5_LIBS)
fi

AC_ARG_WITH(networkmanager, [--with-networkmanager	Use NetworkManager to detect online status], ac_networkmanager=$withval, ac_networkmanager=auto)
if test x"$ac_networkmanager" != xno; then
   PKG_CHECK_MODULES(NETWORKMANAGER, [ NetworkManager >= 0.2 ], have_networkmanager=yes, have_networkmanager=no)
fi
if test x"$ac_networkmanager" = xno; then
   if test x"$have_networkmanager" = xyes; then
      AC_MSG_ERROR([NetworkManager support explicitly requested but not found on system])
   fi
   ac_networkmanager=no
else
   if test x"$have_networkmanager" != xno; then
      AC_DEFINE(HAVE_NETWORKMANAGER, 1, [Define if you have NetworkManager])
      NETWORKMANAGER_CFLAGS="-DDBUS_API_SUBJECT_TO_CHANGE $NETWORKMANAGER_CFLAGS"
      AC_SUBST(NETWORKMANAGER_CFLAGS)
      AC_SUBST(NETWORKMANAGER_LIBS)
   fi
fi

CFLAGS="$GNOME_CFLAGS $KRB5_CFLAGS $NETWORKMANAGER_CFLAGS $CFLAGS"

AC_OUTPUT([
Makefile
krb5-auth-dialog.spec
src/Makefile
etpo/Makefile
po/Makefile.in
])
