NULL =

bin_PROGRAMS = krb5-auth-dialog-preferences

krb5_auth_dialog_preferences_SOURCES = \
	krb5-auth-dialog-preferences.c \
	$(top_srcdir)/src/krb5-auth-gconf-tools.c \
	$(top_srcdir)/src/krb5-auth-gconf-tools.h \
	$(top_srcdir)/src/krb5-auth-tools.c \
	$(top_srcdir)/src/krb5-auth-tools.h \
	$(NULL)

krb5_auth_dialog_preferences_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-DKA_DATA_DIR=\""$(pkgdatadir)"\" \
	-DLOCALE_DIR=\""$(localedir)/"\"
	$(AM_CPPFLAGS)

krb5_auth_dialog_preferences_CFLAGS = \
	$(GTK_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(AM_CFLAGS)


krb5_auth_dialog_preferences_LDADD = \
	$(GCONF_LIBS) \
	$(GTK_LIBS) \
	$(NULL)

AM_CPPFLAGS = \
	$(DISABLE_DEPRECATED) \
	$(NULL)

desktopdir       = $(datadir)/applications
desktop_in_files = krb5-auth-dialog-preferences.desktop.in
desktop_DATA     = $(desktop_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

pkgdatadir = $(datadir)/krb5-auth-dialog
pkgdata_DATA =			\
	krb5-auth-dialog-preferences.xml

CLEANFILES = \
	$(desktop_DATA) \
        $(NULL)

EXTRA_DIST = \
	$(desktop_in_files) \
	$(pkgdata_DATA) \
	$(NULL)
