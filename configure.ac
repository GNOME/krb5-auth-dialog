AC_INIT(src/krb5-auth-dialog.c)

AM_INIT_AUTOMAKE(krb5-auth-dialog, 0.4)
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

dnl NetworkManager

AC_MSG_CHECKING([whether to enable NetworkManager support])
AC_ARG_ENABLE([network-manager],
	AS_HELP_STRING([--enable-network-manager],[Whether to enable automatic network status with NetworkManager]),
	[],[enable_network_manager=autodetect])
AC_MSG_RESULT([$enable_network_manager])

if test "x$enable_network_manager" != "xno"; then
	PKG_CHECK_MODULES([NETWORK_MANAGER],[libnm_glib >= 0.5],
		[enable_network_manager=yes],[enable_network_manager=no])
	AC_SUBST([NETWORK_MANAGER_CFLAGS])
	AC_SUBST([NETWORK_MANAGER_LIBS])
fi

if test "x$enable_network_manager" = "xyes"; then
	AC_DEFINE([ENABLE_NETWORK_MANAGER],[1],[Define for NetworkManager support])
fi

AM_CONDITIONAL([ENABLE_NETWORK_MANAGER],[test "x$enable_network_manager" = "xyes"])

CFLAGS="$GNOME_CFLAGS $KRB5_CFLAGS $NETWORK_MANAGER_CFLAGS $CFLAGS"

AC_OUTPUT([
Makefile
krb5-auth-dialog.spec
src/Makefile
etpo/Makefile
po/Makefile.in
])
