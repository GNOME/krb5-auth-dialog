AC_INIT(src/krb5-auth-dialog.c)

AM_INIT_AUTOMAKE(krb5-auth-dialog, 0.6)
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

AC_PATH_PROG([KRB5_CONFIG], krb5-config, none, $PATH:/usr/kerberos/bin)
if test "x$KRB5_CONFIG" != "xnone"; then
   KRB5_LIBS="`${KRB5_CONFIG} --libs krb5`"
   KRB5_CFLAGS="`${KRB5_CONFIG} --cflags krb5`"
   AC_SUBST(KRB5_CFLAGS)
   AC_SUBST(KRB5_LIBS)
fi

dnl Check for API differences between Heimdal and MIT Kerberos
savedCFLAGS="$CFLAGS"
CFLAGS="$KRB5_CFLAGS $CFLAGS"
AC_CHECK_MEMBERS(krb5_creds.ticket_flags,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.forwardable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.renewable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.proxiable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags,,,[#include <krb5.h>])
AC_MSG_CHECKING(if a krb5_principal->realm is a char*)
AC_COMPILE_IFELSE([
$ac_includes_default
#include <krb5.h>
#include <string.h>
int
main(int argc, char **argv)
{
	static krb5_principal foo;
	return strlen(foo->realm);
}],[AC_DEFINE(HAVE_KRB5_PRINCIPAL_REALM_AS_STRING,1,[Define if the realm of a krb5_principal is a char*])
AC_MSG_RESULT(yes)],
AC_MSG_RESULT(no))

AC_MSG_CHECKING(if a krb5_principal->realm is a krb5_data)
AC_COMPILE_IFELSE([
$ac_includes_default
#include <krb5.h>
int
main(int argc, char **argv)
{
	static krb5_principal foo;
	static krb5_data bar;
	foo->realm = bar;
	return 0;
}],[AC_DEFINE(HAVE_KRB5_PRINCIPAL_REALM_AS_DATA,1,[Define if the realm of a krb5_principal is a krb5_data])
AC_MSG_RESULT(yes)],
AC_MSG_RESULT(no))
CFLAGS="$savedCFLAGS"

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

check_interval=30
AC_DEFINE_UNQUOTED(CREDENTIAL_CHECK_INTERVAL,[$check_interval],
		   [Define the to number of seconds to wait between checks of
		    the Kerberos credential cache.])
AC_SUBST(check_interval)

minimum_lifetime=30
AC_DEFINE_UNQUOTED(MINUTES_BEFORE_PROMPTING,[$minimum_lifetime],
		   [Define the to the minimum amount of time (m) a credential
		    will have to be valid before we'll ask the user to get
		    fresh credentials.])
AC_SUBST(minimum_lifetime)

CFLAGS="$GNOME_CFLAGS $KRB5_CFLAGS $NETWORK_MANAGER_CFLAGS $CFLAGS"

AC_OUTPUT([
Makefile
krb5-auth-dialog.spec
src/Makefile
src/krb5-auth-dialog.1
etpo/Makefile
po/Makefile.in
])
