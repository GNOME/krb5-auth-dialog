INCLUDES =				\
	-DGLADEDIR=\""$(datadir)/krb5-auth-dialog/"\"

bin_PROGRAMS = krb5-auth-dialog

krb5_auth_dialog_SOURCES =	\
	krb5-auth-dialog.c

krb5_auth_dialog_LDADD = @KRB5_LIBS@ @GNOME_LIBS@

gladedir = $(datadir)/krb5-auth-dialog
glade_DATA =			\
	krb5-auth-dialog.glade

EXTRA_DIST =		\
	$(glade_DATA)

