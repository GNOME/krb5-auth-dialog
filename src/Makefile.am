INCLUDES =				\
	-DGLADEDIR=\""$(datadir)/krb5-auth-dialog/"\"

bin_PROGRAMS = krb5-auth-dialog

krb5_auth_dialog_SOURCES =	\
	krb5-auth-dialog.c      \
	dummy-strings.c

krb5_auth_dialog_LDADD = @NETWORK_MANAGER_LIBS@ @KRB5_LIBS@ @GNOME_LIBS@

gladedir = $(datadir)/krb5-auth-dialog
glade_DATA =			\
	krb5-auth-dialog.glade

EXTRA_DIST =		\
	$(glade_DATA)

