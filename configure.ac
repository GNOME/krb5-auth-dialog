AC_INIT([krb5-auth-dialog],[3.15.4])
AC_CONFIG_SRCDIR(src/ka-kerberos.c)
dnl Make automake keep quiet about wildcards & other GNUmake-isms
AM_INIT_AUTOMAKE([1.11.1 no-dist-gzip dist-xz -Wno-portability])
AC_CONFIG_HEADERS([config.h])

AM_MAINTAINER_MODE
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

IT_PROG_INTLTOOL([0.35.0])

GETTEXT_PACKAGE=krb5-auth-dialog
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])

AM_GLIB_GNU_GETTEXT

AC_PROG_CC
AC_SEARCH_LIBS([strerror],[cposix])
AC_HEADER_STDC
LT_INIT
AC_PROG_LEX
AC_PROG_YACC

AM_PROG_CC_C_O

KA_COMPILE_WARNINGS([maximum])
GNOME_DOC_INIT

GTK_REQUIRED=3.14
GLIB_REQUIRED=2.28
LIBNOTIFY_REQUIRED=0.7.0

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= $GTK_REQUIRED, glib-2.0 >= $GLIB_REQUIRED])
PKG_CHECK_MODULES(GIO, [gio-2.0, gio-unix-2.0])
PKG_CHECK_MODULES(GMODULE, gmodule-2.0)
PKG_CHECK_MODULES(LIBNOTIFY, libnotify >= $LIBNOTIFY_REQUIRED)

dnl check for deprecated symbols:
dnl GNOME_MAINTAINER_MODE_DEFINES

GLIB_GSETTINGS

GLIB_COMPILE_RESOURCES=`$PKG_CONFIG --variable glib_compile_resources gio-2.0`
AC_SUBST(GLIB_COMPILE_RESOURCES)

dnl --enable-debug=(yes|no)
AC_ARG_ENABLE(debug,
              AS_HELP_STRING([--enable-debug=no/yes],[enable debugging output]),[],[enable_debug=no])
if test x"$enable_debug" = x"yes"; then
   AC_DEFINE(ENABLE_DEBUG, 1, [whether debugging is enabled])
fi

dnl check for location of opensc-pkcs11 helper used for pkinit
AC_ARG_WITH([pkcs11],
        [AS_HELP_STRING([--with-pkcs11],
          [set path of PKCS11 smartcard helper @<:@default=/usr/lib/opensc/opensc-pkcs11.so@:>@])],
        [],
        [with_pkcs11=/usr/lib/opensc/opensc-pkcs11.so])
SC_PKCS11="$with_pkcs11"
AC_SUBST(SC_PKCS11)
AC_DEFINE_UNQUOTED([SC_PKCS11],["$SC_PKCS11"],[PKCS11 smartcard helper])


AC_PATH_PROG([KRB5_CONFIG], krb5-config, none, $PATH:/usr/kerberos/bin)
if test "x$KRB5_CONFIG" != "xnone"; then
   KRB5_LIBS="`${KRB5_CONFIG} --libs krb5`"
   KRB5_CFLAGS="`${KRB5_CONFIG} --cflags krb5`"
   AC_SUBST(KRB5_CFLAGS)
   AC_SUBST(KRB5_LIBS)
else
   AC_MSG_ERROR([krb5-config not found. You need to install a Kerberos development package])
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
AC_CHECK_FUNCS([krb5_get_error_message \
                krb5_free_error_message \
                krb5_free_error_string \
                krb5_get_renewed_creds \
                krb5_get_init_creds_opt_set_default_flags \
                krb5_cc_clear_mcred])
AC_CHECK_FUNCS([krb5_get_init_creds_opt_set_pkinit],
		[heimdal_pkinit=yes],[heimdal_pkinit=no])
AC_CHECK_FUNCS([krb5_get_init_creds_opt_set_pa],
		[mit_pkinit=yes],[mit_pkinit=no])
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
AC_MSG_RESULT(yes)], AC_MSG_RESULT(no))

dnl pkinit
AC_MSG_CHECKING([whether to enable pkinit support])
AC_ARG_ENABLE([pkinit],
	AS_HELP_STRING([--enable-pkinit],[whether to enable preauth via pkinit support]),
	[],[enable_pkinit=autodetect])

if test "x$heimdal_pkinit" = "xyes" -o \
        "x$mit_pkinit" = "xyes"; then
	enable_pkinit=yes
	AC_DEFINE([ENABLE_PKINIT],[1],[Define for pkinit support])
fi
AC_MSG_RESULT([$enable_pkinit])
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
	PKG_CHECK_MODULES([NETWORK_MANAGER],[libnm-glib >= 0.7.999],
		[enable_network_manager=yes],[enable_network_manager=no])
	AC_SUBST([NETWORK_MANAGER_CFLAGS])
	AC_SUBST([NETWORK_MANAGER_LIBS])
fi

if test "x$enable_network_manager" = "xyes"; then
	AC_DEFINE([ENABLE_NETWORK_MANAGER],[1],[Define for NetworkManager support])
fi
AM_CONDITIONAL([ENABLE_NETWORK_MANAGER],[test "x$enable_network_manager" = "xyes"])


dnl libnotify
savedCFLAGS="$CFLAGS"
savedLIBS="$LIBS"
CFLAGS="$LIBNOTIFY_CFLAGS $CFLAGS"
LIBS="$LIBNOTIFY_LIBS $LIBS"
AC_CHECK_FUNCS([notify_notification_new_with_status_icon \
                notify_notification_set_hint])
CFLAGS="$savedCFLAGS"
LIBS="$savedLIBS"


dnl secmem
dnl Checks for library functions.
AC_CHECK_FUNCS(seteuid stpcpy mmap)
GNUPG_CHECK_MLOCK
GNUPG_CHECK_TYPEDEF(byte, HAVE_BYTE_TYPEDEF)
GNUPG_CHECK_TYPEDEF(ulong, HAVE_ULONG_TYPEDEF)
dnl Check for libcap
AC_ARG_WITH([libcap], AS_HELP_STRING([--without-libcap],[Disable support for capabilities library]))
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
		   [Define to the minimum amount of time (m) a credential
		    will have to be valid before we will ask the user to get
		    fresh credentials.])
AC_SUBST(minimum_lifetime)

dnl
dnl Plugins
dnl
KA_PLUGINS_DIR="$libdir/krb5-auth-dialog/plugins"
AC_SUBST(KA_PLUGINS_DIR)

dnl PAM Plugin
PAM_LIBS=
AC_ARG_WITH([pam],
  AS_HELP_STRING([--with-pam],[use PAM @<:@default=check@:>@]),
  [],
  [with_pam=check])

if test "$with_pam" != "no"; then
  if test "$with_pam" = "check"; then
      AC_CHECK_LIB([pam], [pam_start], [with_pam=yes], [with_pam=no])
  else
      fail=0
      AC_CHECK_LIB([pam], [pam_start], [], [fail=1])
      test $fail = 1 &&
        AC_MSG_ERROR([You must install the PAM development package in order to compile krb5-auth-dialog])
  fi
fi

if test "$with_pam" = "yes"; then
  PAM_LIBS="${PAM_LIBS} -lpam"
fi
AC_SUBST(PAM_LIBS)
AM_CONDITIONAL([ENABLE_PAM_PLUGIN],[test "$with_pam" = "yes"])

dnl Final stage
AC_CONFIG_FILES([
Makefile
krb5-auth-dialog.spec
src/Makefile
src/krb5-auth-dialog.1
secmem/Makefile
icons/Makefile
icons/22x22/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
etpo/Makefile
po/Makefile.in
help/Makefile
examples/Makefile
plugins/Makefile
])
AC_OUTPUT

AC_MSG_NOTICE([])
AC_MSG_NOTICE([Configuration summary])
AC_MSG_NOTICE([=====================])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([Libraries])
AC_MSG_NOTICE([])
AC_MSG_NOTICE([         Kerberos: $KRB5_CFLAGS $KRB5_LIBS])
AC_MSG_NOTICE([              GTK: $GTK_CFLAGS $GTK_LIBS])
AC_MSG_NOTICE([         gio-unix: $GIO_UNIX_CFLAGS $GIO_UNIX_LIBS $GIO_CFLAGS $GIO_LIBS])
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
AC_MSG_NOTICE([Plugins])
AC_MSG_NOTICE([])
if test "$with_pam" = "yes" ; then
AC_MSG_NOTICE([  PAM plugin: $PAM_LIBS])
else
AC_MSG_NOTICE([  PAM plugin: no])
fi
AC_MSG_NOTICE([])
