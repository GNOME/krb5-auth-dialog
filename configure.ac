AC_INIT([krb5-auth-dialog], [0.8])
AC_CONFIG_SRCDIR(src/krb5-auth-dialog.c)
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([-Wno-portability])
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

KA_COMPILE_WARNINGS([maximum])

ALL_LINGUAS="nb de"
AM_GLIB_GNU_GETTEXT

GTK_REQUIRED="2.4.0"
GLADE_REQUIRED="2.4.0"
DBUS_REQUIRED="0.60"
GCONF_REQUIRED="2.8"
LIBNOTIFY_REQUIRED="0.4"
GIO_UNIX_REQUIRED="0.0"

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED)
PKG_CHECK_MODULES(GLADE, libglade-2.0 >= $GLADE_REQUIRED)
PKG_CHECK_MODULES(DBUS, dbus-glib-1 >= $DBUS_REQUIRED)
PKG_CHECK_MODULES(GCONF, gconf-2.0 >= $GCONF_REQUIRED)
PKG_CHECK_MODULES(GIO_UNIX, gio-unix-2.0 >= $GIO_UNIX_REQUIRED)

dnl --enable-debug=(yes|no)
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=no/yes],
                             [enable debugging output]),[],[enable_debug=no])
if test x"$enable_debug" = x"yes"; then
   AC_DEFINE(ENABLE_DEBUG, 1, [whether debugging is enabled])
fi


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
savedLIBS="$LIBS"
LIBS="$KRB5_LIBS $LIBS"
AC_CHECK_HEADERS([hx509_err.h])
AC_CHECK_MEMBERS(krb5_creds.ticket_flags,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.forwardable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.renewable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags.b.proxiable,,,[#include <krb5.h>])
AC_CHECK_MEMBERS(krb5_creds.flags,,,[#include <krb5.h>])
AC_CHECK_FUNCS([krb5_get_error_message])
AC_CHECK_FUNCS([krb5_get_renewed_creds])
AC_CHECK_FUNCS([krb5_get_init_creds_opt_set_default_flags])
AC_CHECK_FUNCS([krb5_cc_clear_mcred])
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
dnl pkinit
AC_MSG_CHECKING([whether to enable pkinit support])
AC_ARG_ENABLE([pkinit],
	AS_HELP_STRING([--enable-pkinit],[whether to enable preauth via pkinit support]),
	[],[enable_pkinit=autodetect])
AC_MSG_RESULT([$enable_pkinit])

if test "x$enable_pkinit" != "xno"; then
	AC_CHECK_FUNCS([krb5_get_init_creds_opt_set_pkinit],
		[enable_pkinit=yes],[enable_pkinit=no])
fi

if test "x$enable_pkinit" = "xyes"; then
	AC_DEFINE([ENABLE_PKINIT],[1],[Define for pkinit support])
fi
AM_CONDITIONAL([ENABLE_PKINIT],[test "x$enable_pkinit" = "xyes"])
CFLAGS="$savedCFLAGS"
LIBS="$savedLIBS"



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

dnl libnotify
LIBNOTIFY_CFLAGS=
LIBNOTIFY_LIBS=
AC_ARG_WITH(libnotify,
  [  --with-libnotify	use libnotify for status messages],
  [],
  [with_libnotify=check])

if test "x$with_libnotify" = "xyes" -o "x$with_libnotify" = "xcheck"; then
  PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED,
    [with_libnotify=yes], [
    if test "x$with_libnotify" = "xcheck" ; then
       with_libnotify=no
    else
       AC_MSG_ERROR(
         [You must install libnotify >= $LIBNOTIFY_REQUIRED to compile krb5-auth-dialog])
    fi
  ])
  if test "x$with_libnotify" = "xyes" ; then
    AC_DEFINE_UNQUOTED(HAVE_LIBNOTIFY, 1,
      [use libnotify for status messages])
  fi
fi
AM_CONDITIONAL(HAVE_LIBNOTIFY, [test "x$with_libnotify" = "xyes"])
AC_SUBST(LIBNOTIFY_CFLAGS)
AC_SUBST(LIBNOTIFY_LIBS)


dnl secmem
dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy mmap)
GNUPG_CHECK_MLOCK
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)
dnl Check for libcap
AC_ARG_WITH([libcap], AC_HELP_STRING([--without-libcap],
            [Disable support for capabilities library]))
if test "x$with_libcap" != "xno"; then
  AC_PATH_PROG(SETCAP, setcap, :, "$PATH:/sbin:/usr/sbin")
  AC_CHECK_LIB(cap, cap_set_proc, [
    AC_DEFINE(USE_CAPABILITIES,1,[The capabilities support library is installed])
    LIBCAP=-lcap
  ])
fi
AC_SUBST(LIBCAP)


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

CFLAGS="\
	$GTK_CFLAGS \
	$GLADE_CFLAGS \
	$DBUS_CFLAGS \
	$GCONF_CFLAGS \
	$KRB5_CFLAGS \
	$NETWORK_MANAGER_CFLAGS \
	$LIBNOTIFY_CFLAGS \
	$WARN_CFLAGS \
	$CFLAGS"

AC_OUTPUT([
Makefile
krb5-auth-dialog.spec
src/Makefile
src/krb5-auth-dialog.1
secmem/Makefile
gtksecentry/Makefile
icons/Makefile
etpo/Makefile
po/Makefile.in
])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Configuration summary])
AC_MSG_NOTICE([=====================])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Libraries])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([         kerberos: $KRB5_CFLAGS $KRB5_LIBS])
AC_MSG_NOTICE([              gtk: $GTK_CFLAGS $GTK_LIBS])
AC_MSG_NOTICE([            glade: $GLADE_CFLAGS $GLADE_LIBS])
AC_MSG_NOTICE([             dbus: $DBUS_CFLAGS $DBUS_LIBS])
AC_MSG_NOTICE([            gconf: $GCONF_CFLAGS $GCONF_LIBS])
AC_MSG_NOTICE([         gio-unix: $GIO_UNIX_CFLAGS $GIO_UNIX_LIBS])
if test "$with_libnotify" = "yes" ; then
AC_MSG_NOTICE([        libnotify: $LIBNOTIFY_CFLAGS $LIBNOTIFY_LIBS])
else
AC_MSG_NOTICE([        libnotify: no])
fi
if test "$enable_network_manager" = "yes" ; then
AC_MSG_NOTICE([  Network Manager: $NETWORK_MANAGER_CFLAGS $NETWORK_MANAGER_LIBS])
else
AC_MSG_NOTICE([  Network Manager: no])
fi
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Miscellaneous])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([          Warnings: $WARN_CFLAGS])
AC_MSG_NOTICE([            Debug : $enable_debug])
AC_MSG_NOTICE([  Minimum Lifetime: $minimum_lifetime minutes])
AC_MSG_NOTICE([    Check Interval: $check_interval seconds])
AC_MSG_NOTICE([])

