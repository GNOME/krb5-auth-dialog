INCLUDES =				\
	-DGLADEDIR=\""$(datadir)/krb5-auth-dialog/"\"

bin_PROGRAMS = krb5-auth-dialog
man_MANS = krb5-auth-dialog.1

autostartdir = $(sysconfdir)/xdg/autostart
autostart_in_files = krb5-auth-dialog.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

krb5_auth_dialog_SOURCES =	\
	krb5-auth-dialog.c      \
	dummy-strings.c

krb5_auth_dialog_LDADD = @NETWORK_MANAGER_LIBS@ @KRB5_LIBS@ @GNOME_LIBS@

gladedir = $(datadir)/krb5-auth-dialog
glade_DATA =			\
	krb5-auth-dialog.glade

EXTRA_DIST =		\
	$(glade_DATA)   \
	$(autostart_in_files) \
	krb5-auth-dialog.1.in

DISTCLEANFILES = krb5-auth-dialog.desktop
